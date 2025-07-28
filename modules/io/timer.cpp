module;

#include <memory>
#include <algorithm>
#include <print>
#include <vector>
#include <iterator>
#include <functional>

#include <uv.h>

export module io:timer;
import :event_scheduler;

namespace poller::io {

struct TimerHandle final {
    uv_timer_t handle_{};
    uint64_t timeout_{};
    uint64_t repeat_{};
    // uv_timer_cb type
    void ( *cb_ )( uv_timer_t *handle );
};

export struct Timer final : EventScheduler {
    auto setTimer( uint64_t timeout, uint64_t repeat,
                   void ( *cb )( uv_timer_t * ) ) -> void {
        //
        auto r = scheduleJob();

        auto t = std::make_unique<TimerHandle>();

        t->timeout_ = timeout;
        t->repeat_ = repeat;
        t->cb_ = cb;

        r->data = t.get();

        uv_async_init( loop_, r, []( uv_async_t *handle ) {
            auto timer = static_cast<TimerHandle *>( handle->data );
            uv_timer_init( handle->loop, &timer->handle_ );
            uv_timer_start( &timer->handle_, timer->cb_, timer->timeout_,
                            timer->repeat_ );

            // Manually close active uv_async_t handle.
            // It exclude this handle from event loop queue.
            uv_close( reinterpret_cast<uv_handle_t *>( handle ),
                      []( uv_handle_t *handle ) {} );
        } );

        uv_async_send( r );

        pool_.push_back( std::move( t ) );
    };

private:
    // Find first active uv_timer_t handle that can be used
    // to perform operation.
    // NOTE: just liniera search.
    auto findActive() -> size_t {
        const auto it =
            std::find_if( pool_.cbegin(), pool_.cend(), []( const auto &item ) {
                return uv_is_active( reinterpret_cast<const uv_handle_t *>(
                           &item ) ) != 0;
            } );

        if ( it != pool_.cend() ) {
            return std::distance( pool_.cbegin(), it );
        } else {
            return -1;
        }
    }

private:
    std::vector<std::unique_ptr<TimerHandle>> pool_;
};

}  // namespace poller::io
