
module;

#include <format>
#include <coroutine>
#include <print>
#include <string>
#include <thread>
#include <chrono>

export module postman;

import poller;
import coro;

namespace postman {

using namespace std::chrono_literals;

#define KEEP_ALIVE true

const std::string POSTMAN_ECHO_GET = "https://postman-echo.com/get";
const std::string POSTMAN_ECHO_GET_ARG_STRING =
    "https://postman-echo.com/get?arg=good_to_see_some_string_here";
const std::string POSTMAN_ECHO_GET_ARG_42 =
    "https://postman-echo.com/get?arg=42";

export struct PostmanClient final {
    PostmanClient() = default;

    PostmanClient( const PostmanClient& other ) = delete;
    PostmanClient( PostmanClient&& other ) = delete;

    auto operator=( const PostmanClient& other ) -> PostmanClient& = delete;
    auto operator=( PostmanClient&& other ) -> PostmanClient& = delete;

    ~PostmanClient() = default;

    auto run() -> void {
        auto coroHandle = getString();

        request();

        requestAsync( POSTMAN_ECHO_GET );

        client_.run();

        // If client.run() not blocks then
        // wait some time until pending requests done.
        if ( KEEP_ALIVE ) {
            std::println( "=== wait for responses..." );
            std::this_thread::sleep_for( 5000ms );
        }
    }

private:
    [[nodiscard]]
    auto getString() -> poller::Task<poller::Result> {
        co_return co_await client_.requestAsyncPromise(
            POSTMAN_ECHO_GET_ARG_STRING );
    }

    auto request() -> poller::Task<void> {
        auto req = poller::HttpRequest{ POSTMAN_ECHO_GET, "agent 0.1" };

        auto resp = co_await client_.requestAsyncVoid( std::move( req ) );

        std::println( "ready: {} - {}", resp.code, resp.data );
    }

    auto requestAsync( std::string rqst ) -> poller::Task<void> {
        auto resp = co_await client_.requestAsyncVoid( rqst );

        std::println( "ready {} - {}", resp.code, resp.data );
    }

private:
    poller::Poller client_{ KEEP_ALIVE };
};

}  // namespace postman