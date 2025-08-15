
#include <iostream>
#include <print>
#include <coroutine>
#include <thread>
#include <chrono>

import poller;

using namespace std::chrono_literals;

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
    auto req =
        poller::HttpRequest{ "https://postman-echo.com/get", "poller/0.2" };

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

    std::println( "request postman-echo.com" );
    requestAsync( "https://postman-echo.com/get" );

    std::println( "request httpbin.org" );
    requestAsync( "http://httpbin.org/user-agent" );

    std::println( "request www.gstatic.com" );
    requestAsync( "http://www.gstatic.com/generate_204" );

    requestAsync( "https://api.coindesk.com/v1/bpi/currentprice.json" );

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
