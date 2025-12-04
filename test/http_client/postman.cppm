
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

namespace postman {

using namespace std::chrono_literals;

const std::string POSTMAN_ECHO_GET = "https://postman-echo.com/get";
const std::string POSTMAN_ECHO_GET_ARG_STRING =
    "https://postman-echo.com/get?arg=good_to_see_some_string_here";
const std::string POSTMAN_ECHO_GET_ARG_42 =
    "https://postman-echo.com/get?arg=42";

export struct PostmanClient final : poller::Poller {
    PostmanClient() = default;

    PostmanClient( const PostmanClient& other ) = delete;
    PostmanClient( PostmanClient&& other ) = delete;

    auto operator=( const PostmanClient& other ) -> PostmanClient& = delete;
    auto operator=( PostmanClient&& other ) -> PostmanClient& = delete;

    ~PostmanClient() = default;

    auto run() -> void override {
        {
            auto req = poller::HttpRequest{};
            req.setUrl( POSTMAN_ECHO_GET );
            request( std::move( req ) );
        }

        {
            auto req = poller::HttpRequest{};
            req.setUrl( POSTMAN_ECHO_GET_ARG_STRING );
            request( std::move( req ) );
        }

#define NUMBER_OF_PROMISES 3
        std::vector<poller::Task<std::pair<int, std::string>>> resps;
        for ( int i = 0; i < NUMBER_OF_PROMISES; ++i ) {
            auto req = poller::HttpRequest{};

            req.setUrl( POSTMAN_ECHO_GET_ARG_42 );

            auto resp = requestPromise( std::move( req ) );

            resps.emplace_back( std::move( resp ) );

            std::print( "=== request {} performed\n", i );
        }

        submit();

        for ( auto&& prom : resps ) {
            // Block until get() result!
            auto [code, data] = prom.get();
            std::print( "=== response code: {} body: {}\n", code, data );
        }

        auto req = poller::HttpRequest{};
        req.setUrl( POSTMAN_ECHO_GET_ARG_STRING );
        auto resp = requestPromiseThenable( std::move( req ) );
        resp.then( []( std::pair<int, std::string> value ) -> void {
            //
            auto [code, data] = value;
            std::print( "=== thenable value: {} body: {}\n", code, data );
        } );

        submit();
    }

private:
    auto request( poller::HttpRequest&& req ) -> poller::Task<void> {
        auto resp = co_await requestAsync<void>( std::move( req ) );

        const auto [code, data, headers] = resp;

        std::println( "response code: {}\ndata:\n{}", code, data );
    }

    [[nodiscard]] auto requestPromise( poller::HttpRequest&& rqst )
        -> poller::Task<std::pair<int, std::string>> {
        auto resp = co_await requestAsync<std::pair<int, std::string>>(
            std::move( rqst ) );

        try {
            const auto respJson = nlohmann::json::parse( resp.data );

            co_return{ resp.code, respJson["args"]["arg"] };
        } catch ( const nlohmann::json::parse_error& e ) {
            std::println(
                "json parse error\n"
                "message:\t{}\n"
                "exception id:\t{}\n"
                "byte position of error:\t{}\n",
                e.what(), e.id, e.byte );

            co_return{ 0, "NO DATA" };
        }
    }

    [[nodiscard]] auto requestPromiseThenable( poller::HttpRequest&& rqst )
        -> poller::ThenableTask<std::pair<int, std::string>> {
        auto resp = co_await requestAsyncThenable<std::pair<int, std::string>>(
            std::move( rqst ) );

        try {
            const auto respJson = nlohmann::json::parse( resp.data );

            co_return{ resp.code, respJson["args"]["arg"] };
        } catch ( const nlohmann::json::parse_error& e ) {
            std::println(
                "json parse error\n"
                "message:\t{}\n"
                "exception id:\t{}\n"
                "byte position of error:\t{}\n",
                e.what(), e.id, e.byte );

            co_return{ 0, "NO DATA" };
        }
    }
};

}  // namespace postman