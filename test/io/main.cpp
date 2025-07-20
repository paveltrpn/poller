
#include <iostream>
#include <print>
#include <thread>
#include <chrono>

import io;

using namespace std::chrono_literals;

auto main( int argc, char** argv ) -> int {
    poller::io::EventScheduler sched;
    sched.passTimer( 1 );
    sched.run();
    std::this_thread::sleep_for( 10s );
    return 0;
}
