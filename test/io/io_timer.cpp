
#include <exception>
#include <print>
#include <thread>
#include <chrono>

#include <uv.h>

import io;

using namespace std::chrono_literals;

auto main( int argc, char** argv ) -> int {
    try {
        poller::io::Timer sched{};
        int counter{ 0 };

#define SLEEP_FOR 1000ms
        std::println( "main thread sleep for: {}", SLEEP_FOR );
        std::this_thread::sleep_for( SLEEP_FOR );

        sched.timeout( 500, []( uv_timer_t* handle ) {
            //
            std::println( "timer fires" );
        } );

        sched.timeout( 500, []( uv_timer_t* handle ) {
            //
            std::println( "timer fires again" );
        } );

        sched.timeout( 2000, []( uv_timer_t* handle ) {
            //
            std::println( "and again" );
        } );

        sched.timeout(
            3000,
            []( uv_timer_t* handle ) {
                auto th = static_cast<poller::io::Timer*>( handle->data );
                std::println( "after 3000 msec pool size: {}",
                              th->handlesCount() );
            },
            &sched );

        sched.timeout(
            1000,
            []( uv_timer_t* handle ) {
                auto th = static_cast<poller::io::Timer*>( handle->data );
                th->handlesInfo();
            },
            &sched );

        sched.repeat(
            250,
            []( uv_timer_t* handle ) {
                //
                auto c = static_cast<int*>( handle->data );
                ++( *c );
                if ( *c > 3 ) {
                    uv_close( reinterpret_cast<uv_handle_t*>( handle ),
                              nullptr );
                }
                std::println( "repeat {}", *c );
            },
            &counter );

        sched.run();
        sched.sync_wait();
    } catch ( std::exception& e ) {
        std::println( "exception is {}", e.what() );
    }

    return 0;
}
