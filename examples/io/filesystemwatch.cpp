
#include <print>
#include <coroutine>
#include <chrono>
#include <thread>
#include <filesystem>

#include "uv.h"

import io;

namespace fs = std::filesystem;
using namespace std::chrono_literals;

poller::io::Scheduler shed{};

auto watch( const std::string &path ) -> poller::io::Task<void> {
    fs::path filePath{ path };

    if ( !fs::exists( filePath ) ) {
        std::println( "File {} not exists! Create some and pass it path as second command line argument!", path );
        co_return;
    }

    std::print(
      "Watch file {}. Change or rename it to see how it works.\nFor example you can append new line to it by executing "
      "command like \"echo \"your string\" >> {}\"\n",
      path, path );

    const auto [status, events] = co_await shed.watchFile( path );

    if ( status < 0 ) {
        std::println( "FS Event error: %s\n", uv_strerror( status ) );
        co_return;
    }

    // fprintf( stdout, "Change detected on path: %s\n", handle->path );
    // if ( filename ) {
    //     fprintf( stdout, "Changed file/directory: %s\n", filename );
    // }

    if ( events & UV_RENAME ) {
        std::println( "Event: Renamed" );
    }

    if ( events & UV_CHANGE ) {
        std::println( "Event: Modified" );
    }
}

auto main( int argc, char **argv ) -> int {
    if ( argc < 2 ) {
        std::println( "pass filename to watch for as second command line argument." );
        return 0;
    }

    if ( argc > 2 ) {
        std::println( "extra command line arguments ignored..." );
    }

    std::this_thread::sleep_for( 100ms );

    watch( std::string{ argv[1] } );

    std::println( "sleep for first time" );
    std::this_thread::sleep_for( 1000ms );

    return 0;
}