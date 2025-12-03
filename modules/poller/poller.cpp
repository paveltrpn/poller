module;

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <stop_token>
#include <string>
#include <thread>
#include <print>
#include <coroutine>
#include <functional>

#include <curl/curl.h>

export module poller:poller;

import coro;
import poller_std;

import :request;
import :handle;
import :write_func;

namespace poller {

export using CallbackFn = std::function<void( Result result )>;

export struct Request {
    CallbackFn callback;
    std::string buffer;
};

[[maybe_unused]] auto writeToRequest( char* ptr, size_t, size_t nmemb,
                                      void* tab ) -> size_t {
    auto r = reinterpret_cast<Request*>( tab );
    r->buffer.append( ptr, nmemb );
    return nmemb;
}

[[maybe_unused]] auto fillRequest( char* ptr, size_t, size_t nmemb,
                                   Request* tab ) -> size_t {
    tab->buffer.append( ptr, nmemb );
    return nmemb;
}

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

    auto submit() -> void {
        worker_.submit( [this]() -> void {
            int msgs_left{};
            int still_running{};

            do {
                // curl perform
                {
                    const auto res =
                        curl_multi_perform( multiHandle_, &still_running );

                    if ( res != CURLM_OK ) {
                        std::println( "curl_multi_perform failed, code {}",
                                      curl_multi_strerror( res ) );
                        break;
                    }
                }

                // curl poll
                {
                    const auto res = curl_multi_poll( multiHandle_, nullptr, 0,
                                                      1000, nullptr );

                    if ( res != CURLM_OK ) {
                        std::println( "curl_multi_poll failed, code {}",
                                      curl_multi_strerror( res ) );
                        break;
                    }
                }

                CURLMsg* msg{};
                do {
                    msg = curl_multi_info_read( multiHandle_, &msgs_left );
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

                        auto requestPtr = (Request*){};
                        {
                            auto privatePtr = (void*){};
                            const auto res = curl_easy_getinfo(
                                handle, CURLINFO_PRIVATE, &privatePtr );

                            if ( res != CURLE_OK ) {
                                std::println(
                                    "curl_easy_getinfo failed, code {}\n",
                                    curl_easy_strerror( res ) );
                            }

                            requestPtr =
                                reinterpret_cast<Request*>( privatePtr );
                        }

                        requestPtr->callback(
                            { code, std::move( requestPtr->buffer ) } );

                        curl_multi_remove_handle( multiHandle_, handle );
                        curl_easy_cleanup( handle );

                        // Delete manually allocated data.
                        delete requestPtr;
                    }
                } while ( msg );
            } while ( still_running );
        } );
    }

    auto performRequest( const HttpRequest& request, CallbackFn cb )
        -> void = delete;

    auto requestAsync( const HttpRequest& request )
        -> RequestAwaitable<HttpRequest, Task<void>> = delete;

    void performRequest( const std::string& url, CallbackFn cb ) {
        auto requestPtr = new Request{ std::move( cb ), {} };

        poller::Handle handle;

        handle.setopt<CURLOPT_URL>( url );
        handle.setopt<CURLOPT_USERAGENT>( POLLER_USERAGNET_STRING );
        handle.setopt<CURLOPT_WRITEFUNCTION>( writeToRequest );
        handle.setopt<CURLOPT_WRITEDATA>( requestPtr );
        handle.setopt<CURLOPT_PRIVATE>( requestPtr );
        handle.setopt<CURLOPT_HEADER>( 1l );

        // POST parameters
        // curl_easy_setopt(curl, CURLOPT_POST,#include <coroutine> 1);
        // const char *urlPOST = "login=ИМЯ&password=ПАСС&cmd=login";
        // curl_easy_setopt(curl, CURLOPT_POSTFIELDS, urlPOST);

        // GET
        // curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        // curl_easy_setopt(curl, CURLOPT_USERPWD, "user:pass");
        // curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.42.0");
        // curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
        // curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        // curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_string);

        curl_multi_add_handle( multiHandle_, handle );
    }

    void performRequest( HttpRequest&& request, CallbackFn cb ) {
        if ( request.isValid() ) {
            // Allocate Requset data. Delete after curl perform actions.
            auto requestPtr = new Request{ std::move( cb ), {} };

            // It is used to set the User-Agent: header field in the
            // HTTP request sent to the remote server.
            request.handle().setopt<CURLOPT_USERAGENT>(
                POLLER_USERAGNET_STRING );

            // This callback function gets called by libcurl as soon as there
            // is data received that needs to be saved. For most transfers,
            // this callback gets called many times and each invoke delivers
            // another chunk of data. ptr points to the delivered data, and
            // the size of that data is nmemb; size is always 1.
            request.handle().setopt<CURLOPT_WRITEFUNCTION>( writeToRequest );

            // If you use the CURLOPT_WRITEFUNCTION option, this is the pointer you
            // get in that callback's fourth and last argument. If you do not use a
            // write callback, you must make pointer a 'FILE ' (cast to 'void ') as
            // libcurl passes this to fwrite(3) when writing data.
            request.handle().setopt<CURLOPT_WRITEDATA>( requestPtr );

            // Pointing to data that should be associated with this curl handle
            request.handle().setopt<CURLOPT_PRIVATE>( requestPtr );

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

    auto requestAsyncVoid( std::string url )
        -> RequestAwaitable<std::string, Task<void>>;

    auto requestAsyncPromise( std::string url )
        -> RequestAwaitable<std::string, Task<Result>>;

    auto requestAsyncVoid( HttpRequest&& request )
        -> RequestAwaitable<HttpRequest, Task<void>>;

    auto requestAsyncPromise( HttpRequest&& request )
        -> RequestAwaitable<HttpRequest, Task<Result>>;

private:
#define LONELEY_THREAD 1
    // curl multi worker thread.
    poller::thread::ThreadPool worker_{ LONELEY_THREAD };

    // Main curl handle
    CURLM* multiHandle_;
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

    // can be void, bool, coroutine_handle<>
    auto await_suspend(
        std::coroutine_handle<typename U::promise_type> handle ) noexcept {
        client_.performRequest( std::move( request_ ),
                                [handle, this]( Result res ) -> void {
                                    result_ = std::move( res );
                                    handle.resume();
                                } );
    }

    [[nodiscard]]
    auto await_resume() const noexcept -> Result {
        //
        return std::move( result_ );
    }

    Poller& client_;
    T request_;
    Result result_;
};

auto Poller::requestAsyncVoid( std::string url )
    -> RequestAwaitable<std::string, Task<void>> {
    //
    return { *this, std::move( url ) };
}

auto Poller::requestAsyncPromise( std::string url )
    -> RequestAwaitable<std::string, Task<Result>> {
    //
    return { *this, std::move( url ) };
}

auto Poller::requestAsyncVoid( HttpRequest&& request )
    -> RequestAwaitable<HttpRequest, Task<void>> {
    //
    return { *this, std::move( request ) };
}

auto Poller::requestAsyncPromise( HttpRequest&& request )
    -> RequestAwaitable<HttpRequest, Task<Result>> {
    //
    return { *this, std::move( request ) };
}

}  // namespace poller
