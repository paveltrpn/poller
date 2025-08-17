
#include <iostream>
#include <print>
#include <coroutine>
#include <thread>
#include <chrono>

#include <nlohmann/json.hpp>

import poller;

using namespace std::chrono_literals;

enum class RequestEndpointEnum {
    POSTMAN_ECHO_GET,
    HTTPBIN_USERAGENT,
    GSTATIC_GENERATE404,
    COINDESK_CURENTPRICE
};

static const std::unordered_map<RequestEndpointEnum, std::string> requests = {
    { RequestEndpointEnum::POSTMAN_ECHO_GET, "https://postman-echo.com/get" },
    { RequestEndpointEnum::HTTPBIN_USERAGENT, "http://httpbin.org/user-agent" },
    { RequestEndpointEnum::GSTATIC_GENERATE404,
      "http://www.gstatic.com/generate_204" },
    { RequestEndpointEnum::COINDESK_CURENTPRICE,
      "https://api.coindesk.com/v1/bpi/currentprice.json" } };

#define KEEP_ALIVE false
// Global Poller client object.
poller::Poller client{ KEEP_ALIVE };

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
        requests.at( RequestEndpointEnum::POSTMAN_ECHO_GET ), "poller/0.2" };

    httpRequestAsync( std::move( req ) );
    httpRequestAsync( std::move( req ) );

    // std::println( "request postman-echo.com" );

    // std::vector<poller::Task<poller::Result>> resps;

    //for ( int i = 0; i < 10; ++i ) {
    //    auto resp = requestPromise(
    //        client, { "https://postman-echo.com/get", "curl coro/0.2" } );

    //    // resps.emplace_back( std::move( resp ) );
    //    std::print( "resp {} performed\n", i );
    //}

    //for ( int i = 0; i < 10; ++i ) {
    //    //const auto [code, data] = resps[i].get();
    //    //std::print( "resp {}\ncode: {}\nbody: {}\n", i, code, data );
    //}

    requestAsync( requests.at( RequestEndpointEnum::POSTMAN_ECHO_GET ) );

    requestAsync( requests.at( RequestEndpointEnum::HTTPBIN_USERAGENT ) );

    requestAsync( requests.at( RequestEndpointEnum::GSTATIC_GENERATE404 ) );

    requestAsync( requests.at( RequestEndpointEnum::COINDESK_CURENTPRICE ) );

    // requestAndStop( client, "https://postman-echo.com/get" );

    client.run();

    // If client.run() not blocks then
    // wait some time until pending requests done.
    if ( KEEP_ALIVE ) {
        std::println( " === wait for responses..." );
        std::this_thread::sleep_for( 500ms );
    }

    return 0;
}