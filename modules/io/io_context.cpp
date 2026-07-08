
module;

#include <cstdlib>
#include <coroutine>

#include <uv.h>

export module io:context;

import :event_scheduler;
import :async;

namespace poller::io {

struct ReadHandle {
    uv_fs_t open_;
    uv_fs_t read_;
    uv_fs_t close_;
    uv_buf_t buf_;
};

struct WriteHandle {
    uv_fs_t open_;
    uv_fs_t write_;
    uv_fs_t close_;
    uv_buf_t buf_;
};

export template <typename T>
struct TimeoutAwaitable;

export struct IoContext final : EventScheduler {
    template <typename T>
    friend struct TimeoutAwaitable;

public:
    // Fire once by timeout.
    auto timeout( uint64_t timeout ) -> TimeoutAwaitable<Task<void>>;
};

template <typename T>
struct TimeoutAwaitable final {
    TimeoutAwaitable( IoContext &context, uint64_t timeout )
        : context_( context )
        , timeout_{ timeout } {};

    [[nodiscard]]
    auto await_ready() const noexcept -> bool {
        //
        return false;
    }

    auto await_suspend( std::coroutine_handle<typename T::promise_type> coroHandle ) noexcept -> void {
        // Execute on event loop.
        auto newTimerTask = []( uv_loop_t *loop, void *coro ) -> void {
            // TODO: delete this somehow!
            auto timer = static_cast<uv_timer_t *>( std::malloc( sizeof( uv_timer_t ) ) );
            auto timeout{ 1000 };
            auto repeat{ 0 };

            uv_handle_set_data( reinterpret_cast<uv_handle_t *>( timer ), coro );

            // Coroutine resume callback.
            auto onFiresCb = []( uv_timer_t *timer ) -> void {
                auto coroHandle = std::coroutine_handle<typename T::promise_type>::from_address( timer->data );

                if ( !coroHandle ) {
                    log::error()( "bad coro handle!" );
                } else {
                    coroHandle.resume();
                }

                uv_close( reinterpret_cast<uv_handle_t *>( timer ), []( uv_handle_t *handle ) -> void {
                    //
                    std::free( handle );
                } );
            };

            uv_timer_init( loop, timer );
            uv_timer_start( timer, onFiresCb, timeout, repeat );
        };

        context_.schedule( newTimerTask, coroHandle.address() );
    }

    auto await_resume() const noexcept -> void {
        //
    }

    IoContext &context_;
    uint64_t timeout_;
};

auto IoContext::timeout( uint64_t timeout ) -> TimeoutAwaitable<Task<void>> {
    //
    return TimeoutAwaitable<Task<void>>{ /* Context */ *this, timeout };
};

}  // namespace poller::io
