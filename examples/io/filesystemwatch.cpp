
#include <print>
#include <coroutine>
#include <chrono>
#include <thread>

#include "uv.h"

import io;

using namespace std::chrono_literals;

poller::io::Scheduler shed{};

auto watch() -> poller::io::Task<void> {
    std::println( "try watch file..." );

    const auto [status, events] = co_await shed.watchFile( "none" );

    if ( status < 0 ) {
        std::println( "FS Event error: %s\n", uv_strerror( status ) );
        co_return;
    }

    // fprintf( stdout, "Change detected on path: %s\n", handle->path );
    // if ( filename ) {
    //     fprintf( stdout, "Changed file/directory: %s\n", filename );
    // }

    if ( events & UV_RENAME ) {
        std::println( "Event: Renamed\n" );
    }

    if ( events & UV_CHANGE ) {
        std::println( "Event: Modified\n" );
    }
}

auto main( int argc, char **argv ) -> int {
    std::this_thread::sleep_for( 100ms );

    watch();

    std::println( "sleep for first time" );
    std::this_thread::sleep_for( 1000ms );

    return 0;
}