
#include <print>
#include <coroutine>
#include <chrono>
#include <thread>

import io;

using namespace std::chrono_literals;

poller::io::Scheduler shed{};

auto delay( uint64_t duration ) -> poller::io::Task<void> {
    std::println( "shchedule timeout..." );
    co_await shed.timeout( duration );
    std::println( "timeout of {} expired!", duration );
}

auto open() -> poller::io::Task<void> {
    std::println( "try open file..." );
    const auto res = co_await shed.openFile( "none" );
    std::println( "file operation result = {}", res );
}

auto main( int argc, char **argv ) -> int {
    std::this_thread::sleep_for( 100ms );

    open();

    delay( 666 );

    std::println( "sleep for first time" );
    std::this_thread::sleep_for( 1000ms );

    return 0;
}
