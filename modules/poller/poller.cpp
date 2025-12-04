module;

#include <string>
#include <print>
#include <coroutine>
#include <memory>

#include <curl/curl.h>

export module poller:poller;

import poller_std;

import :request;
import :handle;
import :write_func;
import :task;
import :payload;
import :result;

namespace poller {

export template <typename T, typename U>
struct RequestAwaitable;

#define POLLER_USERAGNET_STRING "poller/0.1"

export struct Poller {
public:
    Poller() {
        // Curl global init.
        {
            const auto res = curl_global_init( CURL_GLOBAL_DEFAULT );
            if ( res != CURLE_OK ) {
                std::println( "curl_global_init failed, code {}\n",
                              curl_easy_strerror( res ) );
                throw std::runtime_error( "curl_global_init failed" );
            }
        }

        multiHandle_ = curl_multi_init();
        if ( !multiHandle_ ) {
            std::println( "can't create curl multi handle" );
            throw std::runtime_error( "curl_multi_init failed" );
        }
    }

    Poller( const Poller& other ) = delete;
    Poller( Poller&& other ) = delete;
    auto operator=( const Poller& other ) -> Poller& = delete;
    auto operator=( Poller&& other ) -> Poller& = delete;

    ~Poller() {
        worker_.wait();

        // We already left curl multi loop.
        curl_multi_cleanup( multiHandle_ );
        curl_global_cleanup();
    }

    template <TaskParamter T>
    auto requestAsync( const HttpRequest& request )
        -> RequestAwaitable<HttpRequest, Task<T>> = delete;

    template <TaskParamter T>
    auto requestAsync( HttpRequest&& request )
        -> RequestAwaitable<HttpRequest, Task<T>>;

    virtual auto run() -> void = 0;

protected:
    auto submit() -> void {
        worker_.submit( [this]() -> void {
            int msgsLeft{};
            int stillRunning{};

            do {
                // Curl perform.
                {
                    const auto res =
                        curl_multi_perform( multiHandle_, &stillRunning );

                    if ( res != CURLM_OK ) {
                        std::println( "curl_multi_perform failed, code {}",
                                      curl_multi_strerror( res ) );
                        break;
                    }
                }

                // Curl poll.
                {
// TODO: make it part of public api.
#define MULTI_POLL_TIMEOUT 1000
                    const auto res = curl_multi_poll(
                        multiHandle_, nullptr, 0, MULTI_POLL_TIMEOUT, nullptr );

                    if ( res != CURLM_OK ) {
                        std::println( "curl_multi_poll failed, code {}",
                                      curl_multi_strerror( res ) );
                        break;
                    }
                }

                CURLMsg* msg{};
                do {
                    msg = curl_multi_info_read( multiHandle_, &msgsLeft );
                    if ( msg && ( msg->msg == CURLMSG_DONE ) ) {
                        CURL* handle = msg->easy_handle;

                        long code{};
                        {
                            const auto res = curl_easy_getinfo(
                                handle, CURLINFO_RESPONSE_CODE, &code );
                            if ( res != CURLE_OK ) {
                                std::println(
                                    "curl_easy_getinfo failed, code {}\n",
                                    curl_easy_strerror( res ) );
                            }
                        }

                        // Auto free when out of scope.
                        auto rpPtr = std::unique_ptr<Payload>{};
                        {
                            auto privatePtr = (void*){};
                            const auto res = curl_easy_getinfo(
                                handle, CURLINFO_PRIVATE, &privatePtr );

                            if ( res != CURLE_OK ) {
                                std::println(
                                    "curl_easy_getinfo failed, code {}\n",
                                    curl_easy_strerror( res ) );
                            }

                            rpPtr.reset(
                                reinterpret_cast<Payload*>( privatePtr ) );
                        }

                        rpPtr->callback( { code, std::move( rpPtr->data ),
                                           std::move( rpPtr->headers ) } );

                        curl_multi_remove_handle( multiHandle_, handle );
                        curl_easy_cleanup( handle );
                    }
                } while ( msg );
            } while ( stillRunning );
        } );
    }

private:
    auto performRequest( const HttpRequest& request, CallbackFn cb )
        -> void = delete;

    auto performRequest( const std::string& url, CallbackFn cb ) const -> void {
        auto rp = new Payload{ std::move( cb ), {}, {} };

        poller::Handle handle;

        // URL for this transfer.
        handle.setopt<CURLOPT_URL>( url );

        handle.setopt<CURLOPT_USERAGENT>( POLLER_USERAGNET_STRING );

        handle.setopt<CURLOPT_WRITEFUNCTION>( writeDataCallback );

        handle.setopt<CURLOPT_WRITEDATA>( rp );

        // Callback that receives header data.
        handle.setopt<CURLOPT_HEADERFUNCTION>( writeHeaderCallback );

        // Pointer to pass to header callback
        handle.setopt<CURLOPT_HEADERDATA>( rp );

        handle.setopt<CURLOPT_PRIVATE>( rp );

        curl_multi_add_handle( multiHandle_, handle );
    }

    auto performRequest( HttpRequest&& request, CallbackFn cb ) const -> void {
        if ( request.isValid() ) {
            // Allocate Requset data. Delete after curl perform actions.
            auto rp = new Payload{ std::move( cb ), {}, {} };

            // It is used to set the User-Agent: header field in the
            // HTTP request sent to the remote server.
            request.handle().setopt<CURLOPT_USERAGENT>(
                POLLER_USERAGNET_STRING );

            // This callback function gets called by libcurl as soon as there
            // is data received that needs to be saved. For most transfers,
            // this callback gets called many times and each invoke delivers
            // another chunk of data. ptr points to the delivered data, and
            // the size of that data is nmemb; size is always 1.
            request.handle().setopt<CURLOPT_WRITEFUNCTION>( writeDataCallback );

            // If you use the CURLOPT_WRITEFUNCTION option, this is the pointer you
            // get in that callback's fourth and last argument. If you do not use a
            // write callback, you must make pointer a 'FILE ' (cast to 'void ') as
            // libcurl passes this to fwrite(3) when writing data.
            request.handle().setopt<CURLOPT_WRITEDATA>( rp );

            // Callback that receives header data.
            request.handle().setopt<CURLOPT_HEADERFUNCTION>(
                writeHeaderCallback );

            // Pointer to pass to header callback
            request.handle().setopt<CURLOPT_HEADERDATA>( rp );

            // Pointing to data that should be associated with this curl handle
            request.handle().setopt<CURLOPT_PRIVATE>( rp );

            // Ask libcurl to include the headers in the write callback (CURLOPT_WRITEFUNCTION).
            // This option is relevant for protocols that actually have headers
            // or other meta-data (like HTTP and FTP)
            //
            // request.handle().setopt<CURLOPT_HEADER>( 1l );

            // Is this thrad safe to add easy handle to multi
            // when multi handle performing loop cuntinues???
            //
            // "You can add more easy handles to a multi handle at any
            // point, even if other transfers are already running"
            curl_multi_add_handle( multiHandle_, request );

            // Clean request allocated data (headers slist pointer etc.)
            request.clean();
        } else {
            std::println( "poller request not performed, request is invalid!" );
        }
    }

private:
#define LONELEY_THREAD 1
    // curl multi worker thread.
    poller::thread::ThreadPool worker_{ LONELEY_THREAD };

    // Main curl handle
    CURLM* multiHandle_;

    template <typename T, typename U>
    friend struct RequestAwaitable;
};

export template <typename T, typename U>
struct RequestAwaitable final {
    RequestAwaitable( Poller& client, T request )
        : client_( client )
        , request_( std::move( request ) ) {};

    // HTTP request always NOT ready immedieateley!
    [[nodiscard]]
    auto await_ready() const noexcept -> bool {
        return false;
    }

    auto await_suspend(
        std::coroutine_handle<typename U::promise_type> handle ) noexcept {
        client_.performRequest( std::move( request_ ),
                                [handle, this]( Result res ) -> void {
                                    result_ = std::move( res );
                                    handle.resume();
                                } );
    }

    [[nodiscard]]
    auto await_resume() noexcept -> Result {
        //
        return std::move( result_ );
    }

private:
    const Poller& client_;
    T request_;
    Result result_;
};

template <TaskParamter T>
auto Poller::requestAsync( HttpRequest&& request )
    -> RequestAwaitable<HttpRequest, Task<T>> {
    //
    return { *this, std::move( request ) };
}

}  // namespace poller
