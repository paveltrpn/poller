
module;

#include <cstdlib>
#include <coroutine>
#include <memory>

#include <uv.h>

export module io:timer;

import :scheduler;
import :payload;
import :async;

namespace poller::io {

export template <typename T>
struct TimeoutAwaitable final {
    TimeoutAwaitable( Scheduler &context, uint64_t timeout )
        : context_( context )
        , payload_{ std::shared_ptr<TimeoutCbPayload>( new TimeoutCbPayload{ nullptr, timeout } ) } {};

    [[nodiscard]]
    auto await_ready() const noexcept -> bool {
        //
        return false;
    }

    auto await_suspend( std::coroutine_handle<typename T::promise_type> coroHandle ) noexcept -> void {
        // Execute on event loop.
        auto newTimerTask = []( uv_loop_t *loop, AsyncJobPayload *payload ) -> void {
            auto timer = static_cast<uv_timer_t *>( std::malloc( sizeof( uv_timer_t ) ) );

            auto p = static_cast<TimeoutCbPayload *>( payload );
            auto timeout{ p->timeout };
            auto repeat{ 0 };

            uv_handle_set_data( reinterpret_cast<uv_handle_t *>( timer ), payload );

            // Coroutine resume callback.
            auto onFiresCb = []( uv_timer_t *timer ) -> void {
                auto payload = static_cast<AsyncJobPayload *>( timer->data );
                auto coroHandle = std::coroutine_handle<typename T::promise_type>::from_address( payload->coro );

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

        payload_->coro = coroHandle.address();

        context_.schedule( newTimerTask, payload_.get() );
    }

    auto await_resume() const noexcept -> void {
        //
    }

    Scheduler &context_;
    // Store TimeoutCbPayload by base class ptr.
    std::shared_ptr<AsyncJobPayload> payload_{};
};

auto Scheduler::timeout( uint64_t timeout ) -> TimeoutAwaitable<Task<void>> {
    //
    return TimeoutAwaitable<Task<void>>{ /* Context */ *this, timeout };
};

}  // namespace poller::io