
module;

#include <format>
#include <coroutine>
#include <print>
#include <string>
#include <thread>
#include <chrono>
#include <vector>

#include <nlohmann/json.hpp>

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
        requestAsync( POSTMAN_ECHO_GET );

        requestAsync( POSTMAN_ECHO_GET_ARG_STRING );

        std::vector<poller::Task<poller::Result>> resps;
        for ( int i = 0; i < 3; ++i ) {
            auto resp =
                requestPromise( { POSTMAN_ECHO_GET_ARG_42, USER_AGENT } );

            resps.emplace_back( std::move( resp ) );

            std::print( " === request {} performed\n", i );
        }

        client_.submit();

        for ( auto&& prom : resps ) {
            const auto [code, data] = prom.get();
            std::print( " ==== response code: {} body: {}\n", code, data );
        }
    }

private:
    auto requestAsync( std::string rqst ) -> poller::Task<void> {
        auto resp = co_await client_.requestAsyncVoid( rqst );

        std::println( "ready {} - {}", resp.code, resp.data );
    }

    [[nodiscard]] auto requestPromise( poller::HttpRequest&& rqst )
        -> poller::Task<poller::Result> {
        auto resp = co_await client_.requestAsyncPromise( std::move( rqst ) );

        try {
            const auto respJson = nlohmann::json::parse( resp.data );
            co_return{ resp.code, respJson["args"]["arg"] };
        } catch ( const nlohmann::json::parse_error& e ) {
            std::println(
                "config json parse error\n"
                "message:\t{}\n"
                "exception id:\t{}\n"
                "byte position of error:\t{}\n",
                e.what(), e.id, e.byte );
            co_return{ 0, "0" };
        }
    }

private:
    poller::Poller client_{};
};

}  // namespace postman