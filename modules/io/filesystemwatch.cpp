module;

#include <print>
#include <string>
#include <cstdlib>
#include <coroutine>
#include <memory>

#include <uv.h>

export module io:filewatch;

import :scheduler;
import :payload;
import :async;

namespace poller::io {

template <typename T>
struct FilesystemWatchAwaitable final {
    FilesystemWatchAwaitable( Scheduler &context, std::string path )
        : context_( context )
        , payload_{ std::shared_ptr<FilesystemWatchCbPayload>(
            new FilesystemWatchCbPayload{ nullptr, std::move( path ), 0, 0 } ) } {};

    [[nodiscard]] auto await_ready() const noexcept -> bool {
        //
        return false;
    }

    auto await_suspend( std::coroutine_handle<typename T::promise_type> coroHandle ) -> void {
        auto newFilesystemWatchTask = []( uv_loop_t *loop, AsyncJobPayload *payload ) -> void {
            auto onFSEvent = []( uv_fs_event_t *handle, const char *filename, int events, int status ) -> void {
                auto payload = static_cast<FilesystemWatchCbPayload *>( handle->data );
                payload->status = status;
                payload->events = events;

                // Stop the handle, the callback will no longer be called.
                uv_fs_event_stop( handle );

                // Remove handle.
                uv_close( reinterpret_cast<uv_handle_t *>( handle ), []( uv_handle_t *handle ) -> void {
                    //
                    std::free( handle );
                } );

                // Awake coroutine.
                auto coroHandle = std::coroutine_handle<typename T::promise_type>::from_address( payload->coro );

                if ( !coroHandle ) {
                    log::error()( "bad coro handle!" );
                } else {
                    coroHandle.resume();
                }
            };

            auto watcher = static_cast<uv_fs_event_t *>( std::malloc( sizeof( uv_fs_event_t ) ) );

            uv_fs_event_init( loop, watcher );

            uv_handle_set_data( reinterpret_cast<uv_handle_t *>( watcher ), payload );

            auto p = static_cast<FilesystemWatchCbPayload *>( payload );

            // "Callback passed to uv_fs_event_start() which will be called repeatedly
            // after the handle is started." therefore we need to stop them manually.
            uv_fs_event_start( watcher, onFSEvent, p->path.c_str(), 0 );
        };

        payload_->coro = coroHandle.address();

        context_.schedule( newFilesystemWatchTask, payload_.get() );
    }

    auto await_resume() noexcept -> std::tuple<int, int> {
        auto p = static_cast<FilesystemWatchCbPayload *>( payload_.get() );
        return std::make_tuple( p->status, p->events );
    }

    Scheduler &context_;
    // Store FilesystemWatchCbPayload by base class ptr.
    std::shared_ptr<AsyncJobPayload> payload_{};
};

auto Scheduler::watchFile( const std::string &path ) -> FilesystemWatchAwaitable<Task<void>> {
    //
    return FilesystemWatchAwaitable<Task<void>>{ /* Context */ *this, path };
};

}  // namespace poller::io
