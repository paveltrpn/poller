
module;

#include <string>
#include <coroutine>
#include <exception>
#include <uv.h>

export module io:async;

import log;

namespace tire::io {

export template <typename T>
struct Task;

export template <>
struct Task<void> {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {
        Task get_return_object() { return Task<void>{ handle_type::from_promise( *this ) }; };

        auto initial_suspend() noexcept { return std::suspend_never{}; }
        auto final_suspend() noexcept { return std::suspend_never{}; }
        void return_void() {}
        void unhandled_exception() { exception_ = std::current_exception(); }

        std::exception_ptr exception_{ nullptr };
        bool scheduleDestroy_{ false };
    };

    explicit Task( handle_type h )
        : handle_( h ) {}

    Task() = default;

    Task( Task &&t ) noexcept
        : handle_( t.handle_ ) {
        t.handle_ = nullptr;
    }

    Task &operator=( Task &&other ) noexcept {
        if ( std::addressof( other ) != this ) {
            // Destroy this handle, but it allowed only when
            // coroutine is suspended.
            if ( handle_ ) {
                handle_.destroy();
            }

            handle_ = other.handle_;
            other.handle_ = nullptr;
        }

        return *this;
    }

    Task( const Task & ) = delete;
    Task &operator=( const Task & ) = delete;

    ~Task() = default;

    void detach() noexcept {
        if ( empty() ) {
            return;
        }

        if ( handle_ ) {
            handle_.destroy();
            handle_ = nullptr;
        }
    }

    [[nodiscard]] constexpr bool empty() const noexcept { return handle_ == nullptr; }

    constexpr explicit operator bool() const noexcept { return !empty(); }

    void scheduleDestroy() { handle_.promise().scheduleDestroy_ = true; };

private:
    handle_type handle_{ nullptr };
};

/*
template <typename T>
struct TimeoutAwaitable final {
    TimeoutAwaitable( uv_loop_t* loop, long long timeout )
        : loop_( loop )
        , timeout_{ timeout } {};

    [[nodiscard]] bool await_ready() const noexcept { return false; }

    // Can be void, bool, coroutine_handle<>.
    void await_suspend(
        std::coroutine_handle<typename T::promise_type> handle ) noexcept {
        auto cb = []( uv_timer_t* timer ) -> void {
            auto handle =
                static_cast<std::coroutine_handle<typename T::promise_type>*>(
                    timer->data );
            handle->resume();
        };

        auto promise = handle.promise();
        if ( promise.scheduleDestroy_ ) {
            log::notice(
                "TimeoutAwaitable === coroutine destruction pending..." );
            handle.destroy();
            // Returns control to the caller/resumer of the current coroutine
            return;
        }

        handle_ = handle;
        timer_.data = &handle_;
        uv_timer_init( loop_, &timer_ );
        uv_timer_start( &timer_, cb, timeout_, 0 );
    }

    void await_resume() const noexcept {}

    std::coroutine_handle<typename T::promise_type> handle_;
    uv_loop_t* loop_;
    long long timeout_;
    uv_timer_t timer_;
};
*/

template <typename T>
struct FilesystemWatchAwaitable final {
    FilesystemWatchAwaitable( uv_loop_t *loop, const std::string &path )
        : loop_( loop )
        , path_{ path } {};

    [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }

    void await_suspend( std::coroutine_handle<typename T::promise_type> handle ) noexcept {
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

}  // namespace tire::io
