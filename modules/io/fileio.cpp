
module;

#include <print>
#include <string>
#include <cstdlib>
#include <coroutine>

#include <uv.h>

export module io:fileio;

import :scheduler;
import :async;

namespace poller::io {

export template <typename T>
struct FileIOAwaitable final {
    FileIOAwaitable( Scheduler &context, std::string path )
        : context_( context )
        , path_{ std::move( path ) } {};

    [[nodiscard]]
    auto await_ready() const noexcept -> bool {
        //
        return false;
    }

    auto await_suspend( std::coroutine_handle<typename T::promise_type> coroHandle ) noexcept -> void {
        // Execute on event loop.
        auto newFileIOTask = []( uv_loop_t *loop, void *coro ) -> void {
            auto openRqst = static_cast<uv_fs_t *>( std::malloc( sizeof( uv_fs_t ) ) );

            uv_handle_set_data( reinterpret_cast<uv_handle_t *>( openRqst ), coro );

            // Coroutine resume callback.
            auto onFiresCb = []( uv_fs_t *openRqst ) -> void {
                auto coroHandle = std::coroutine_handle<typename T::promise_type>::from_address( openRqst->data );

                if ( !coroHandle ) {
                    log::error()( "bad coro handle!" );
                } else {
                    coroHandle.resume();
                }

                uv_close( reinterpret_cast<uv_handle_t *>( openRqst ), []( uv_handle_t *handle ) -> void {
                    //
                    std::free( handle );
                } );
            };

            int r = uv_fs_open( loop, openRqst, "example.txt", O_RDONLY, 0, onFiresCb );
            if ( r < 0 ) {
                std::println( "Failed to initiate open: {}", uv_strerror( r ) );
                return;
            }
        };

        context_.schedule( newFileIOTask, coroHandle.address() );
    }

    auto await_resume() const noexcept -> void {
        //
    }

    Scheduler &context_;
    std::string path_{};
};

auto Scheduler::openFile( const std::string &path ) -> FileIOAwaitable<Task<void>> {
    //
    return FileIOAwaitable<Task<void>>{ /* Context */ *this, path };
};

}  // namespace poller::io
