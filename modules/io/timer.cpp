module;

#include <memory>
#include <algorithm>
#include <print>
#include <vector>
#include <iterator>
#include <functional>
#include <memory>

#include <uv.h>

export module io:timer;

import :event_scheduler;
import container;

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
        // Acquire uv_async handle in wich this job will be performed.
        auto r = scheduleJob();

        auto t = std::make_shared<TimerHandle>();
        t->timeout_ = timeout;
        t->repeat_ = repeat;
        t->cb_ = cb;

        r->data = t.get();

        // Prepare job.
        uv_async_init( loop_, r, []( uv_async_t *handle ) {
            auto timer = static_cast<TimerHandle *>( handle->data );

            uv_timer_init( handle->loop, &timer->handle_ );
            uv_timer_start( &timer->handle_, timer->cb_, timer->timeout_,
                            timer->repeat_ );

            // Manually close active uv_async_t handle.
            // It exclude this handle from event loop queue.
            uv_close( reinterpret_cast<uv_handle_t *>( handle ), nullptr );
        } );

        // Start job.
        uv_async_send( r );

        pool_.append( std::move( t ) );
    };

    auto printInfo() -> void {
        ia_.data = this;

        uv_async_init( loop_, &ia_, []( uv_async_t *handle ) {
            auto th = static_cast<Timer *>( handle->data );

            th->it_.data = th;

            uv_timer_init( handle->loop, &th->it_ );
            uv_timer_start(
                &th->it_,
                []( uv_timer_t *handle ) {
                    //
                    auto th = static_cast<Timer *>( handle->data );
                    th->info();
                },
                1000, 0 );
            uv_close( reinterpret_cast<uv_handle_t *>( handle ), nullptr );
        } );

        uv_async_send( &ia_ );
    }

    auto info() -> void {
        std::println( "active timers info" );
        pool_.for_each( []( std::shared_ptr<TimerHandle> item ) {
            //
            std::println( "is active = {}",
                          uv_is_active( reinterpret_cast<const uv_handle_t *>(
                              item.get() ) ) );
        } );
    }

private:
    // Find first active uv_timer_t handle that can be used
    // to perform operation.
    // NOTE: just linear search.
    auto findActive() -> size_t {
        auto it = pool_.find_if( []( std::shared_ptr<TimerHandle> item ) {
            return uv_is_active( reinterpret_cast<const uv_handle_t *>(
                       item.get() ) ) != 0;
        } );

        if ( it != pool_.cend() ) {
            return std::distance( pool_.begin(), it );
        } else {
            return -1;
        }
    }

private:
    uv_timer_t it_{};
    uv_async_t ia_{};

    poller::list<TimerHandle> pool_;
};

}  // namespace poller::io
