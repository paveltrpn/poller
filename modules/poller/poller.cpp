
module;

#include <atomic>

#include <string>
#include <thread>
#include <print>
#include <coroutine>

#include <curl/curl.h>

export module poller:poller;

import :async;
import :request;
import :handle;

namespace poller {

[[maybe_unused]] static size_t writeToBuffer( char* data, size_t size,
                                              size_t nmemb,
                                              std::string* buffer ) {
    size_t result{};
    if ( buffer != nullptr ) {
        buffer->append( data, size * nmemb );
        result = size * nmemb;
    }
    return result;
}

[[maybe_unused]] static size_t writeToRequest( char* ptr, size_t, size_t nmemb,
                                               void* tab ) {
    auto r = reinterpret_cast<Request*>( tab );
    r->buffer.append( ptr, nmemb );
    return nmemb;
}

[[maybe_unused]] static size_t fillRequest( char* ptr, size_t, size_t nmemb,
                                            Request* tab ) {
    tab->buffer.append( ptr, nmemb );
    return nmemb;
}

template <typename T>
struct Task;

export template <typename T, typename U>
struct RequestAwaitable;

export struct Poller {
public:
    Poller() {
        CURLcode res{};
        res = curl_global_init( CURL_GLOBAL_DEFAULT );
        if ( res != CURLE_OK ) {
            std::println( "curl_global_init failed, code {}\n",
                          curl_easy_strerror( res ) );
            throw std::runtime_error( "curl_global_init failed" );
        }

        multiHandle_ = curl_multi_init();
        if ( !multiHandle_ ) {
            std::println( "can't create curl multi handle" );
            throw std::runtime_error( "curl_multi_init failed" );
        }

        worker_ = std::make_unique<std::thread>( &Poller::run, this );
        worker_->detach();
    }

    ~Poller() {
        stop();
        curl_multi_cleanup( multiHandle_ );
        curl_global_cleanup();
    }

    Poller( const Poller& other ) = delete;
    Poller( Poller&& other ) = delete;
    Poller& operator=( const Poller& other ) = delete;
    Poller& operator=( Poller&& other ) = delete;

    void stop() {
        break_ = true;
        curl_multi_wakeup( multiHandle_ );
    }

    void performRequest( const HttpRequest& request, CallbackFn cb ) = delete;

    RequestAwaitable<HttpRequest, Task<void>> requestAsync(
        const HttpRequest& request ) = delete;

    void performRequest( const std::string& url, CallbackFn cb ) {
        Request* requestPtr = new Request{ std::move( cb ), {} };
        poller::Handle handle;
        handle.setopt<CURLOPT_URL>( url );
        handle.setopt<CURLOPT_USERAGENT>( "poller/0.1" );
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
            Request* requestPtr = new Request{ std::move( cb ), {} };

            request.handle().setopt<CURLOPT_WRITEFUNCTION>( writeToRequest );
            request.handle().setopt<CURLOPT_WRITEDATA>( requestPtr );
            request.handle().setopt<CURLOPT_PRIVATE>( requestPtr );

            curl_multi_add_handle( multiHandle_, request );
        } else {
            std::println( "poller request not performed, request is invalid!" );
        }
    }

    RequestAwaitable<std::string, Task<void>> requestAsyncVoid(
        std::string url );

    RequestAwaitable<std::string, Task<Result>> requestAsyncPromise(
        std::string url );

    RequestAwaitable<HttpRequest, Task<void>> requestAsyncVoid(
        HttpRequest&& request );

    RequestAwaitable<HttpRequest, Task<Result>> requestAsyncPromise(
        HttpRequest&& request );

private:
    auto run() -> void {
        int msgs_left;
        int still_running = 1;

        while ( !break_ ) {
            CURLMcode res{};

            res = curl_multi_perform( multiHandle_, &still_running );

            if ( res != CURLM_OK ) {
                std::println( "curl_multi_perform failed, code {}",
                              curl_multi_strerror( res ) );
                break;
            }

            res = curl_multi_poll( multiHandle_, nullptr, 0, 1000, nullptr );

            if ( res != CURLM_OK ) {
                std::println( "curl_multi_poll failed, code {}",
                              curl_multi_strerror( res ) );
                break;
            }

            CURLMsg* msg;
            do {
                msg = curl_multi_info_read( multiHandle_, &msgs_left );
                if ( msg && ( msg->msg == CURLMSG_DONE ) ) {
                    CURL* handle = msg->easy_handle;
                    CURLcode res{};

                    int code{};
                    res = curl_easy_getinfo( handle, CURLINFO_RESPONSE_CODE,
                                             &code );
                    if ( res != CURLE_OK ) {
                        std::println( "curl_easy_getinfo failed, code {}\n",
                                      curl_easy_strerror( res ) );
                    }

                    Request* requestPtr{};
                    res = curl_easy_getinfo( handle, CURLINFO_PRIVATE,
                                             &requestPtr );

                    if ( res != CURLE_OK ) {
                        std::println( "curl_easy_getinfo failed, code {}\n",
                                      curl_easy_strerror( res ) );
                    }

                    requestPtr->callback(
                        { code, std::move( requestPtr->buffer ) } );
                    curl_multi_remove_handle( multiHandle_, handle );
                    curl_easy_cleanup( handle );
                    delete requestPtr;
                }
            } while ( !break_ && msg );
        }

        std::println( "curl loop stopped!" );
    }

private:
    std::unique_ptr<std::thread> worker_;

    CURLM* multiHandle_;
    std::atomic_bool break_{ false };
};

export template <typename T, typename U>
struct RequestAwaitable final {
    RequestAwaitable( Poller& client, T request )
        : client_( client )
        , request_( std::move( request ) ) {};

    // HTTP request always NOT ready immedieateley!
    bool await_ready() const noexcept { return false; }

    // can be void, bool, coroutine_handle<>
    auto await_suspend(
        std::coroutine_handle<typename U::promise_type> handle ) noexcept {
        client_.performRequest( std::move( request_ ),
                                [handle, this]( Result res ) {
                                    result_ = std::move( res );
                                    handle.resume();
                                } );
    }

    Result await_resume() const noexcept { return std::move( result_ ); }

    Poller& client_;
    T request_;
    Result result_;
};

RequestAwaitable<std::string, Task<void>> Poller::requestAsyncVoid(
    std::string url ) {
    //
    return { *this, std::move( url ) };
}

RequestAwaitable<std::string, Task<Result>> Poller::requestAsyncPromise(
    std::string url ) {
    //
    return { *this, std::move( url ) };
}

RequestAwaitable<HttpRequest, Task<void>> Poller::requestAsyncVoid(
    HttpRequest&& request ) {
    //
    return { *this, std::move( request ) };
}

RequestAwaitable<HttpRequest, Task<Result>> Poller::requestAsyncPromise(
    HttpRequest&& request ) {
    //
    return { *this, std::move( request ) };
}

}  // namespace poller
