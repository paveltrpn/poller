
#include <iostream>
#include <print>
#include <thread>
#include <chrono>

import io;

using namespace std::chrono_literals;

auto main( int argc, char** argv ) -> int {
    poller::io::SimpleLoop sched;

    sched.setCloseTimer( 1000 );
    sched.printThreadInfo();

    sched.run();

    return 0;
}
