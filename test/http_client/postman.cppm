
module;

#include <format>
#include <coroutine>
#include <print>
#include <string>
#include <thread>
#include <chrono>
#include <vector>

export module postman;

import poller;
import coro;

namespace postman {

using namespace std::chrono_literals;

#define USER_AGENT "poller/0.2"

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
        // auto coroHandle = getString();

        requestAsync( POSTMAN_ECHO_GET );

        requestAsync( POSTMAN_ECHO_GET_ARG_STRING );

        requestAsync( POSTMAN_ECHO_GET_ARG_42 );

        client_.submit();

        // std::vector<poller::Task<poller::Result>> resps;
        // for ( int i = 0; i < 3; ++i ) {
        // auto resp = requestPromise( { POSTMAN_ECHO_GET, USER_AGENT } );
        //
        // resps.emplace_back( std::move( resp ) );
        //
        // std::print( "resp {} performed\n", i );
        // }
        //
        // for ( auto&& prom : resps ) {
        // const auto [code, data] = prom.get();
        // std::print( "got code: {} body: {}\n", code, data );
        // }
    }

private:
    [[nodiscard]]
    auto getString() -> poller::Task<poller::Result> {
        co_return co_await client_.requestAsyncPromise(
            POSTMAN_ECHO_GET_ARG_STRING );
    }

    auto requestAsync( std::string rqst ) -> poller::Task<void> {
        auto resp = co_await client_.requestAsyncVoid( rqst );

        std::println( "ready {} - {}", resp.code, resp.data );
    }

    [[nodiscard]] auto requestPromise( poller::HttpRequest&& rqst )
        -> poller::Task<poller::Result> {
        auto resp = co_await client_.requestAsyncPromise( std::move( rqst ) );
        co_return resp;
    }

private:
    poller::Poller client_{};
};

}  // namespace postman