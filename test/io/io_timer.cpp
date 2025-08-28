
#include <exception>
#include <print>
#include <thread>
#include <chrono>
#include <memory>
#include <uv.h>
#include <coroutine>

import io;
import coro;

using namespace std::chrono_literals;

std::unique_ptr<poller::io::Timer> timer;

auto testTimeout( uint64_t timeout ) -> poller::Task<void> {
    co_await timer->timeout( timeout );
    std::println( "coroutine resumed after {}", timeout );
}

auto printHandlesCount( uint64_t timeout ) -> poller::Task<void> {
    co_await timer->timeout( timeout );
    std::println( "after {} msec timers pool size: {}", timeout,
                  timer->handlesCount() );
}

auto printHandlesInfo( uint64_t timeout ) -> poller::Task<void> {
    co_await timer->timeout( timeout );
    timer->handlesInfo();
}

auto main( int argc, char** argv ) -> int {
    try {
        timer = std::make_unique<poller::io::Timer>();
    } catch ( std::exception& e ) {
        std::println( "exception is {}", e.what() );
    }

    int counter{ 0 };

    testTimeout( 150 );

    timer->timeout( 250, []( uv_timer_t* handle ) {
        //
        std::println( "timer fires" );
    } );

    timer->timeout( 500, []( uv_timer_t* handle ) {
        //
        std::println( "timer fires again" );
    } );

    std::println( "main thread waits..." );
    //std::this_thread::sleep_for( 2500ms );
    std::println( "main thread start..." );

    printHandlesCount( 1000 );

    printHandlesInfo( 3000 );

    timer->repeat(
        300,
        []( uv_timer_t* handle ) {
            //
            auto c = static_cast<int*>( handle->data );
            ++( *c );
            if ( *c > 3 ) {
                uv_close( reinterpret_cast<uv_handle_t*>( handle ), nullptr );
            }
            std::println( "repeat {}", *c );
        },
        &counter );

    std::this_thread::sleep_for( 4000ms );

    return 0;
}
