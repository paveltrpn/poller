
#include <iostream>
#include <print>
#include <coroutine>
#include <thread>
#include <chrono>

#include <nlohmann/json.hpp>

import poller;

using namespace std::chrono_literals;

#define USER_AGENT "poller/0.2"

enum class RequestEndpointEnum {
    POSTMAN_ECHO_GET,
    POSTMAN_ECHO_GET_ARG_STRING,
    POSTMAN_ECHO_GET_ARG_42,
    HTTPBIN_USERAGENT,
    GSTATIC_GENERATE404,
    COINDESK_CURENTPRICE
};

static const std::unordered_map<RequestEndpointEnum, std::string> requests = {
    { RequestEndpointEnum::POSTMAN_ECHO_GET, "https://postman-echo.com/get" },
    { RequestEndpointEnum::POSTMAN_ECHO_GET_ARG_STRING,
      "https://postman-echo.com/get?arg=good_to_see_some_string_here" },
    { RequestEndpointEnum::POSTMAN_ECHO_GET_ARG_42,
      "https://postman-echo.com/get?arg=42" },
    { RequestEndpointEnum::HTTPBIN_USERAGENT, "http://httpbin.org/user-agent" },
    { RequestEndpointEnum::GSTATIC_GENERATE404,
      "http://www.gstatic.com/generate_204" },
    { RequestEndpointEnum::COINDESK_CURENTPRICE,
      "https://api.coindesk.com/v1/bpi/currentprice.json" } };

#define KEEP_ALIVE true
// Global Poller client object.
poller::Poller client{ KEEP_ALIVE };

auto postmanGetString() -> poller::Task<poller::Result> {
    co_return co_await client.requestAsyncPromise(
        requests.at( RequestEndpointEnum::POSTMAN_ECHO_GET_ARG_STRING ) );
}

auto requestAsync( std::string rqst ) -> poller::Task<void> {
    auto resp = co_await client.requestAsyncVoid( rqst );
    std::println( "ready {} - {}", resp.code, resp.data );
}

auto httpRequestAsync( poller::HttpRequest&& rqst ) -> poller::Task<void> {
    auto resp = co_await client.requestAsyncVoid( std::move( rqst ) );
    std::println( "ready: {} - {}", resp.code, resp.data );
}

[[nodiscard]] auto requestPromise( poller::HttpRequest&& rqst )
    -> poller::Task<poller::Result> {
    auto resp = co_await client.requestAsyncPromise( std::move( rqst ) );
    co_return resp;
}

auto requestAndStop( std::string rqst ) -> poller::Task<void> {
    auto resp = co_await client.requestAsyncVoid( std::move( rqst ) );

    std::println( "got response and shutdown" );

    client.stop();
}

auto main( int argc, char** argv ) -> int {
    auto req = poller::HttpRequest{
        requests.at( RequestEndpointEnum::POSTMAN_ECHO_GET ), USER_AGENT };

    httpRequestAsync( std::move( req ) );

    // req in moved-from state
    // httpRequestAsync( std::move( req ) );

    {
        auto resp = postmanGetString();
        const auto [_, r] = resp.get();
        std::println( "=== postman request string: {}", r );
    }

    std::vector<poller::Task<poller::Result>> resps;
    for ( int i = 0; i < 3; ++i ) {
        auto resp =
            requestPromise( { "https://postman-echo.com/get", USER_AGENT } );

        resps.emplace_back( std::move( resp ) );

        std::print( "resp {} performed\n", i );
    }

    for ( auto&& prom : resps ) {
        const auto [code, data] = prom.get();
        std::print( "got code: {} body: {}\n", code, data );
    }

    requestAsync( requests.at( RequestEndpointEnum::POSTMAN_ECHO_GET ) );

    requestAsync( requests.at( RequestEndpointEnum::HTTPBIN_USERAGENT ) );

    requestAsync( requests.at( RequestEndpointEnum::GSTATIC_GENERATE404 ) );

    requestAsync( requests.at( RequestEndpointEnum::COINDESK_CURENTPRICE ) );

    // requestAndStop( client, "https://postman-echo.com/get" );

    // noop if KEEP_ALIVE
    client.run();

    // If client.run() not blocks then
    // wait some time until pending requests done.
    if ( KEEP_ALIVE ) {
        std::println( "=== wait for responses..." );
        std::this_thread::sleep_for( 5000ms );
    }

    return 0;
}