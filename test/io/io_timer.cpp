
#include <print>
#include <uv.h>

import io;

auto main( int argc, char** argv ) -> int {
    poller::io::Timer sched{};

    sched.setTimer( 100, 0, []( uv_timer_t* handle ) {
        //
        std::println( "timer fires" );
    } );

    sched.setTimer( 200, 0, []( uv_timer_t* handle ) {
        //
        std::println( "timer fires again" );
    } );

    sched.sync_wait();

    return 0;
}
