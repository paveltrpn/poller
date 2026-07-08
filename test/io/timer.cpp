
#include <print>
#include <coroutine>
#include <chrono>
#include <thread>

import io;

using namespace std::chrono_literals;

poller::io::IoContext shed{};

auto delay() -> poller::io::Task<void> {
    co_await shed.timeout( 1000 );
    std::println( "timeout expired!" );
}

auto main( int argc, char **argv ) -> int {
    delay();

    shed.run();

    std::this_thread::sleep_for( 2000ms );

    return 0;
}
