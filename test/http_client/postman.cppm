
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

const std::string POSTMAN_ECHO_MASTER_STARTED =
    "https://postman-echo.com/get?arg=master_started";

const std::string POSTMAN_ECHO_SLAVE_STARTED =
    "https://postman-echo.com/get?arg=slave_started";

const std::string POSTMAN_ECHO_SLAVE_DO_JOB =
    "https://postman-echo.com/get?arg=im_totally_done";

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

        {
            auto req = poller::HttpRequest{};
            req.setUrl( POSTMAN_ECHO_GET_ARG_STRING );
            auto resp = requestPromise( std::move( req ) );
            resp.then( []( std::pair<int, std::string> value ) -> void {
                //
                auto [code, data] = value;
                std::print( "=== thenable value: {} body: {}\n", code, data );
            } );
        }

        submit();

#define NUMBER_OF_PROMISES 3
        std::vector<poller::BlockingTask<std::pair<int, std::string>>> resps;
        for ( int i = 0; i < NUMBER_OF_PROMISES; ++i ) {
            auto req = poller::HttpRequest{};

            req.setUrl( POSTMAN_ECHO_GET_ARG_42 );

            auto resp = requestPromiseBlocking( std::move( req ) );

            resps.emplace_back( std::move( resp ) );

            std::print( "=== request {} performed\n", i );
        }

        submit();

        for ( auto&& prom : resps ) {
            // Block until get() result!
            auto [code, data] = prom.get();
            std::print( "=== response code: {} body: {}\n", code, data );
        }

        requestMaster();
        requestSlave();

        submit();
        wait();

        std::println( "shared stated touched {} times", sharedState_ );
    }

private:
    auto parsePostmanGetArg( const std::string& response ) -> std::string {
        try {
            const auto respJson = nlohmann::json::parse( response );
            return respJson["args"]["arg"];
        } catch ( const nlohmann::json::parse_error& e ) {
            std::println(
                "json parse error\n"
                "message:\t{}\n"
                "exception id:\t{}\n"
                "byte position of error:\t{}\n",
                e.what(), e.id, e.byte );

            return {};
        }
    }

    [[maybe_unused]]
    auto request( poller::HttpRequest&& req ) -> poller::Task<void> {
        auto resp = co_await requestAsync<void>( std::move( req ) );

        sharedState_++;

        const auto [code, data, headers] = resp;

        std::println( "response code: {}\ndata:\n{}", code, data );
    }

    [[nodiscard]] auto requestPromise( poller::HttpRequest&& rqst )
        -> poller::Task<std::pair<int, std::string>> {
        sharedState_++;

        auto resp = co_await requestAsync<std::pair<int, std::string>>(
            std::move( rqst ) );

        const auto [code, data, headers] = resp;

        const auto arg = parsePostmanGetArg( data );

        co_return{ code, arg };
    }

    [[nodiscard]] auto requestPromiseBlocking( poller::HttpRequest&& rqst )
        -> poller::BlockingTask<std::pair<int, std::string>> {
        sharedState_++;

        auto resp = co_await requestAsyncBlocking<std::pair<int, std::string>>(
            std::move( rqst ) );

        const auto [code, data, headers] = resp;

        const auto arg = parsePostmanGetArg( data );

        co_return{ code, arg };
    }

    auto requestMaster() -> poller::Task<void> {
        {
            auto req = poller::HttpRequest{};
            req.setUrl( POSTMAN_ECHO_MASTER_STARTED );
            auto resp = co_await requestAsync<void>( std::move( req ) );

            const auto [code, data, headers] = resp;
            const auto arg = parsePostmanGetArg( data );

            println( "=== reset event [ code {}, msg \"{}\" ]", code, arg );
        }

        std::println( "=== reset event [ \"master request job...\" ]" );
        masterBarrier_.set();

        std::println( "=== reset event [ \"master awaits job...\" ]" );
        co_await slaveBarrier_;

        std::println( "=== reset event [ \"master got slave job {}\" ]",
                      slaveJobPayload_ );
    }

    auto requestSlave() -> poller::Task<void> {
        {
            auto req = poller::HttpRequest{};
            req.setUrl( POSTMAN_ECHO_SLAVE_STARTED );
            auto resp = co_await requestAsync<void>( std::move( req ) );

            const auto [code, data, headers] = resp;
            const auto arg = parsePostmanGetArg( data );

            println( "=== reset event [ code {}, msg \"{}\" ]", code, arg );
        }

        std::println( "=== reset event [ \"slave awaits permission...\"] " );
        co_await masterBarrier_;
        std::println( "=== reset event [ \"slave got permission\"!] " );

        {
            auto req = poller::HttpRequest{};
            req.setUrl( POSTMAN_ECHO_SLAVE_DO_JOB );
            auto resp = co_await requestAsync<void>( std::move( req ) );

            const auto [code, data, headers] = resp;
            slaveJobPayload_ = parsePostmanGetArg( data );

            slaveBarrier_.set();
        }
    }

private:
    long long sharedState_{};

    poller::ResetEvent masterBarrier_{};
    poller::ResetEvent slaveBarrier_{};
    std::string slaveJobPayload_{};
};

}  // namespace postman