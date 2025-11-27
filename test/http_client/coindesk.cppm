
module;

#include <format>
#include <coroutine>
#include <print>
#include <string>
#include <thread>
#include <chrono>

export module coindesk;

import poller;
import coro;

namespace coindesk {

using namespace std::chrono_literals;

#define KEEP_ALIVE true

const std::string COINDESK_CURENTPRICE =
    "https://api.coindesk.com/v1/bpi/currentprice.json";

export struct CoindeskClient final {
    CoindeskClient() = default;

    CoindeskClient( const CoindeskClient& other ) = delete;
    CoindeskClient( CoindeskClient&& other ) = delete;

    auto operator=( const CoindeskClient& other ) -> CoindeskClient& = delete;
    auto operator=( CoindeskClient&& other ) -> CoindeskClient& = delete;

    ~CoindeskClient() = default;

    auto run() -> void {
        //
        requestAsync( COINDESK_CURENTPRICE );

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

}  // namespace coindesk
