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
    FilesystemWatchAwaitable( uv_loop_t *loop, std::string path )
        : loop_( loop )
        , path_{ std::move( path ) } {};

    [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }

    auto await_suspend( std::coroutine_handle<typename T::promise_type> handle ) -> void {
        auto cb = []( uv_fs_event_t *watcher, const char *filename, int events, int status ) -> void {
            auto self = static_cast<FilesystemWatchAwaitable<T> *>( watcher->data );
            self->event_ = static_cast<uv_fs_event>( events );
            self->handle_.resume();
        };

        auto promise = handle.promise();
        if ( promise.scheduleDestroy_ ) {
            log::notice()( "coroutine destruction pending..." );
            handle.destroy();
            return;
        }

        watcher_.data = this;
        handle_ = handle;
        uv_fs_event_init( loop_, &watcher_ );
        // "Callback passed to uv_fs_event_start() which will be called repeatedly
        // after the handle is started." therefore we need to stop them manually.
        uv_fs_event_start( &watcher_, cb, path_.c_str(), 0 );
    }

    auto await_resume() noexcept -> uv_fs_event {
        // Stop the handle, the callback will no longer be called.
        uv_fs_event_stop( &watcher_ );
        return event_;
        // uv_close( reinterpret_cast<uv_handle_t *>( &watcher_ ), nullptr );
    }

    std::coroutine_handle<typename T::promise_type> handle_;
    uv_loop_t *loop_;
    std::string path_;
    uv_fs_event_t watcher_;
    uv_fs_event event_{};
};

}  // namespace poller::io
