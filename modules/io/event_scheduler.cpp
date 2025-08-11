module;

#include <memory>
#include <iostream>
#include <thread>
#include <cstdlib>
#include <mutex>
#include <vector>
#include <new>

#include <uv.h>

export module io:event_scheduler;

import container;

namespace poller::io {

// Base class that holds uv loop handle and loop thread. This
// class manages async handles that used for "thread safe" adding
// other usefull uv handles (such as timers, file and network handles)
// to event loop and perform actions.
struct EventScheduler {
    EventScheduler() {
        // Try to allocate main loop handle.
        const auto tmp = malloc( sizeof( uv_loop_t ) );
        if ( !tmp ) {
            std::bad_alloc{};
        }

        loop_ = static_cast<uv_loop_t *>( tmp );

        //
        const auto ret = uv_loop_init( loop_ );
        std::println( "loop init with code {}", ret );

        // Loop thread initialization.
        thread_ = std::make_unique<std::thread>( [this]() {
            // Main thread must explicitly call run() to start event loop.
            std::unique_lock<std::mutex> lk{ m_ };
            cv_.wait( lk, [this]() {
                //
                return run_;
            } );

            // Start event loop.
            uv_run( loop_, UV_RUN_DEFAULT );
        } );
    }

    // Not copyable not moveable "service-like" object.
    EventScheduler( const EventScheduler &other ) = delete;
    EventScheduler( EventScheduler &&other ) = delete;
    EventScheduler &operator=( const EventScheduler &other ) = delete;
    EventScheduler &operator=( EventScheduler &&other ) = delete;

    virtual ~EventScheduler() {
        uv_loop_close( loop_ );
        // Release loop pointer.
        free( loop_ );
    }

    // Manually start event loop thread.
    auto run() -> void {
        {
            std::lock_guard<std::mutex> _{ m_ };
            run_ = true;
        }
        cv_.notify_one();
    }

    // Wait until event loop finished.
    auto sync_wait() -> void {
        //
        thread_->join();
    }

protected:
    auto stop() {
        //
        uv_stop( loop_ );
    }

    auto findActiveJob() -> size_t {
        auto it = std::find_if(
            pendingQueue_.begin(), pendingQueue_.end(), []( auto &item ) {
                return uv_is_active( reinterpret_cast<const uv_handle_t *>(
                           item.get() ) ) != 0;
            } );

        if ( it != pendingQueue_.cend() ) {
            return std::distance( pendingQueue_.begin(), it );
        } else {
            return -1;
        }
    }

    // Add new uv_async_t handle in queue and submit callback to event loop within it.
    // That async handle contain a pointer to specific data, that will be
    // used in callback and callback to perform action on event loop thread itself.
    auto schedule( void *h, std::invocable<uv_async_t *> auto cb ) -> void {
        auto j = std::make_shared<uv_async_t>();

        // Store payload pointer in uv_async_t handle
        j->data = h;

        // Initilaize us_async_t handle with specific payload and callback.
        uv_async_init( loop_, j.get(), cb );

        // Submit job to event loop.
        uv_async_send( j.get() );

        // Save async handle.
        pendingQueue_.append( std::move( j ) );
    }

private:
    // Main and only uv loop handle.
    uv_loop_t *loop_{};

    // Loop thread.
    std::unique_ptr<std::thread> thread_;

    //
    std::condition_variable cv_;
    std::mutex m_;
    bool run_{ false };

    // Async handles that used for calling callbacks
    // on loop thread.
    poller::list<uv_async_t> pendingQueue_{};
};

}  // namespace poller::io
