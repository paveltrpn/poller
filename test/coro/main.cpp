
#include <iostream>
#include <print>

#include "poller.h"
#include "async.h"
#include "request.h"

struct S {
    unsigned i : 2, j : 6;
};

auto requestAsync( poller::Poller& client, std::string rqst )
    -> poller::Task<void> {
    auto resp = co_await client.requestAsyncVoid( rqst );
    std::println( "ready {} - {}", resp.code, resp.data );
}

auto httpRequestAsync( poller::Poller& client, poller::HttpRequest&& rqst )
    -> poller::Task<void> {
    auto resp = co_await client.requestAsyncVoid( std::move( rqst ) );
    std::println( "ready: {} - {}", resp.code, resp.data );
}

[[nodiscard]] auto requestPromise( poller::Poller& client,
                                   poller::HttpRequest&& rqst )
    -> poller::Task<poller::Result> {
    auto resp = co_await client.requestAsyncPromise( std::move( rqst ) );
    co_return resp;
}

auto main( int argc, char** argv ) -> int {
    poller::Poller client;

    auto req =
        poller::HttpRequest{ "https://postman-echo.com/get", "curl coro/0.2" };

    httpRequestAsync( client, std::move( req ) );
    httpRequestAsync( client, std::move( req ) );

    // std::println( "request postman-echo.com" );

    std::vector<poller::Task<poller::Result>> resps;

    for ( int i = 0; i < 10; ++i ) {
        auto resp = requestPromise(
            client, { "https://postman-echo.com/get", "curl coro/0.2" } );

        resps.emplace_back( std::move( resp ) );
        std::print( "resp {} performed\n", i );
    }

    for ( int i = 0; i < 10; ++i ) {
        //const auto [code, data] = resps[i].get();
        //std::print( "resp {}\ncode: {}\nbody: {}\n", i, code, data );
    }

    std::println( "request postman-echo.com" );
    requestAsync( client, "https://postman-echo.com/get" );

    std::println( "request httpbin.org" );
    requestAsync( client, "http://httpbin.org/user-agent" );

    std::println( "request www.gstatic.com" );
    requestAsync( client, "http://www.gstatic.com/generate_204" );

    requestAsync( client, "https://api.coindesk.com/v1/bpi/currentprice.json" );

    std::cin.get();

    return 0;
}
