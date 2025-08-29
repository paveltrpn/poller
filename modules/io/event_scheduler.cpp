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

import poller_std;

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

        loop_ = static_cast<uv_loop_t*>( tmp );

        // We not yet in loop thread. Calling this is safe.
        const auto ret = uv_loop_init( loop_ );

        // Loop thread initialization.
        thread_ = std::make_unique<std::thread>( [this]() {
            // Main thread must explicitly call notify this
            // thread to start event loop.
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
    EventScheduler( const EventScheduler& other ) = delete;
    EventScheduler( EventScheduler&& other ) = delete;
    EventScheduler& operator=( const EventScheduler& other ) = delete;
    EventScheduler& operator=( EventScheduler&& other ) = delete;

    virtual ~EventScheduler() {
        // We already left loop thread.
        // De-initialize and free the resources associated with an event loop.
        uv_loop_close( loop_ );

        // Release loop pointer.
        free( loop_ );
    }

    auto syncWait() -> void {
        {
            std::lock_guard<std::mutex> _{ m_ };
            run_ = true;
        }
        cv_.notify_one();

        //
        thread_->join();
    }

protected:
    // Add new uv_async_t handle in queue and submit callback to event loop within it.
    // That async handle contain a pointer to specific data, that will be
    // used in callback and callback to perform action on event loop thread itself.
    auto schedule( void* payload, std::invocable<uv_async_t*> auto cb )
        -> void {
        // Allocate uv_async_t handle
        auto j = std::make_shared<uv_async_t>();

        // Store payload pointer in async handle
        j->data = payload;

        // Initilaize async handle with specific payload and callback.
        // NOTE: this is (probably) not thread safe!
        uv_async_init( loop_, j.get(), cb );

        // Submit job to event loop.
        // This thread safe (one and only thread safe call across
        // all lobuv API)
        uv_async_send( j.get() );

        // Store handle.
        pendingQueue_.append( std::move( j ) );
    }

private:
    // Main and only uv loop handle.
    uv_loop_t* loop_{};

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
