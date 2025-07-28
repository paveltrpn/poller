
#include <print>
#include <thread>
#include <chrono>

#include <uv.h>

import io;

using namespace std::chrono_literals;

auto main( int argc, char** argv ) -> int {
    poller::io::Timer sched{};

    std::println("main thread sleep...");
    std::this_thread::sleep_for( 1000ms );

    sched.setTimer( 100, 0, []( uv_timer_t* handle ) {
        //
        std::println( "timer fires" );
    } );

    sched.setTimer( 200, 0, []( uv_timer_t* handle ) {
        //
        std::println( "timer fires again" );
    } );

    sched.setTimer( 2000, 0, []( uv_timer_t* handle ) {
        //
        std::println( "and again" );
    } );

    sched.setTimer( 3000, 0, []( uv_timer_t* handle ) {
        //
        std::println( "and again" );
    } );

    sched.printInfo();

    sched.run();
    sched.sync_wait();

    return 0;
}
