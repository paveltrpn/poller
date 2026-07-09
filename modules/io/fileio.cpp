
module;

#include <print>
#include <string>
#include <cstdlib>
#include <coroutine>

#include <uv.h>

export module io:fileio;

import :scheduler;
import :payload;
import :async;

namespace poller::io {

export template <typename T>
struct FileIOAwaitable final {
    FileIOAwaitable( Scheduler &context, std::string path )
        : context_( context )
        , payload_{ nullptr, std::move( path ), 0 } {};

    [[nodiscard]]
    auto await_ready() const noexcept -> bool {
        //
        return false;
    }

    auto await_suspend( std::coroutine_handle<typename T::promise_type> coroHandle ) noexcept -> void {
        // Execute on event loop.
        auto newFileIOTask = []( uv_loop_t *loop, FileIOCbPayload *payload ) -> void {
            auto openRqst = static_cast<uv_fs_t *>( std::malloc( sizeof( uv_fs_t ) ) );

            uv_handle_set_data( reinterpret_cast<uv_handle_t *>( openRqst ), payload );

            // Coroutine resume callback.
            auto onFiresCb = []( uv_fs_t *openRqst ) -> void {
                auto payload = static_cast<FileIOCbPayload *>( openRqst->data );

                payload->opentResult = openRqst->result;

                // std::println( "Open error: {}", uv_strerror( openRqst->result ) );

                // File descriptor can be read, written, or closed here.
                // uv_file opened_fd = openRqst->result;
                // std::println( "File successfully opened with FD: {}", opened_fd );

                // Must always clean up the request.
                uv_fs_req_cleanup( openRqst );

                auto coroHandle = std::coroutine_handle<typename T::promise_type>::from_address( payload->coro );

                if ( !coroHandle ) {
                    log::error()( "bad coro handle!" );
                } else {
                    coroHandle.resume();
                }
            };

            int r = uv_fs_open( loop, openRqst, payload->path.c_str(), O_RDONLY, 0, onFiresCb );
            if ( r < 0 ) {
                std::println( "Failed to initiate open: {}", uv_strerror( r ) );
                return;
            }
        };

        payload_.coro = coroHandle.address();

        context_.scheduleFileIO( newFileIOTask, &payload_ );
    }

    [[nodiscard]]
    auto await_resume() const noexcept -> size_t {
        //
        return payload_.opentResult;
    }

    Scheduler &context_;
    FileIOCbPayload payload_{};
};

auto Scheduler::openFile( const std::string &path ) -> FileIOAwaitable<Task<void>> {
    //
    return FileIOAwaitable<Task<void>>{ /* Context */ *this, path };
};

}  // namespace poller::io
