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
import poller_std;

namespace poller::io {

struct TimerHandle final {
    uv_timer_t handle_{};
    uint64_t timeout_{};
    uint64_t repeat_{};
    // uv_timer_cb type
    void ( *cb_ )( uv_timer_t* handle );
};

export struct Timer final : EventScheduler {
    // Start timer repeating with interval.
    auto repeat( uint64_t repeat, void ( *cb )( uv_timer_t* ),
                 void* payload = nullptr ) -> void {
        //
        auto t = std::make_shared<TimerHandle>();

        //
        t->timeout_ = 0;
        t->repeat_ = repeat;
        t->cb_ = cb;

        // NOTE: can be NULL!!!
        t->handle_.data = payload;

        schedule( t.get(), []( uv_async_t* handle ) {
            auto timer = static_cast<TimerHandle*>( handle->data );

            uv_timer_init( handle->loop, &timer->handle_ );
            uv_timer_start( &timer->handle_, timer->cb_, timer->timeout_,
                            timer->repeat_ );

            // Manually close active uv_async_t handle.
            // It exclude this handle from event loop queue.
            uv_close( reinterpret_cast<uv_handle_t*>( handle ), nullptr );
        } );

        pool_.append( std::move( t ) );
    };

    // Fire once by timeout.
    auto timeout( uint64_t timeout, void ( *cb )( uv_timer_t* ),
                  void* payload = nullptr ) -> void {
        //
        auto t = std::make_shared<TimerHandle>();

        //
        t->timeout_ = timeout;
        t->repeat_ = 0;
        t->cb_ = cb;

        //
        t->handle_.data = payload;

        schedule( t.get(), []( uv_async_t* handle ) {
            auto timer = static_cast<TimerHandle*>( handle->data );

            uv_timer_init( handle->loop, &timer->handle_ );
            uv_timer_start( &timer->handle_, timer->cb_, timer->timeout_,
                            timer->repeat_ );

            uv_close( reinterpret_cast<uv_handle_t*>( handle ), nullptr );
        } );

        pool_.append( std::move( t ) );
    };

    auto handlesInfo() const -> void {
        const auto total = handlesCount();
        int active{ 0 };

        pool_.for_each( [&active]( std::shared_ptr<TimerHandle> item ) {
            if ( uv_is_active(
                     reinterpret_cast<const uv_handle_t*>( item.get() ) ) ) {
                ++active;
            }
        } );

        std::println( "timers info: {}/{} (total/active)", total, active );
    }

    auto handlesCount() const -> size_t {
        //
        return pool_.size();
    }

private:
    // Find first active uv_timer_t handle that can be used
    // to perform operation.
    auto findActive() -> size_t {
        auto it = pool_.find_if( []( std::shared_ptr<TimerHandle> item ) {
            return uv_is_active( reinterpret_cast<const uv_handle_t*>(
                       item.get() ) ) != 0;
        } );

        if ( it != pool_.cend() ) {
            return std::distance( pool_.begin(), it );
        } else {
            return -1;
        }
    }

private:
    // Timer handles.
    poller::list<TimerHandle> pool_;
};

}  // namespace poller::io
