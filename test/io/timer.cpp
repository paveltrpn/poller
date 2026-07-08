
#include <print>
#include <coroutine>
#include <chrono>
#include <thread>

import io;

using namespace std::chrono_literals;

poller::io::IoContext shed{};

auto delay() -> poller::io::Task<void> {
    std::println( "shchedule timeout" );
    co_await shed.timeout( 500 );
    std::println( "timeout expired!" );
}

auto main( int argc, char **argv ) -> int {
    std::println( "sleep for zero time" );
    std::this_thread::sleep_for( 2000ms );

    delay();
    delay();

    std::println( "sleep for first time" );
    std::this_thread::sleep_for( 3000ms );

    delay();

    std::println( "sleep for second time" );
    std::this_thread::sleep_for( 3000ms );

    return 0;
}
