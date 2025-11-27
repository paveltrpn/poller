
module;

#include <format>
#include <coroutine>
#include <print>
#include <string>
#include <thread>
#include <chrono>

export module httpbin;

import poller;
import coro;

namespace httpbin {

using namespace std::chrono_literals;

#define KEEP_ALIVE true

const std::string HTTPBIN_USERAGENT = "http://httpbin.org/user-agent";

export struct HttpbinClient final {
    HttpbinClient() = default;

    HttpbinClient( const HttpbinClient& other ) = delete;
    HttpbinClient( HttpbinClient&& other ) = delete;

    auto operator=( const HttpbinClient& other ) -> HttpbinClient& = delete;
    auto operator=( HttpbinClient&& other ) -> HttpbinClient& = delete;

    ~HttpbinClient() = default;

    auto run() -> void {
        //
        requestAsync( HTTPBIN_USERAGENT );

        client_.run();

        if ( KEEP_ALIVE ) {
            std::println( "=== wait for responses..." );
            std::this_thread::sleep_for( 5000ms );
        }
    }

private:
    auto requestAsync( std::string rqst ) -> poller::Task<void> {
        auto resp = co_await client_.requestAsyncVoid( rqst );
        std::println( "ready {} - {}", resp.code, resp.data );
    }

private:
    poller::Poller client_{ KEEP_ALIVE };
};

}  // namespace httpbin