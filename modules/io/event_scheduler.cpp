
module;

#include <thread>
#include <mutex>
#include "uv.h"
#include <condition_variable>

export module io:event_scheduler;

import log;

import :list;

namespace tire::io {

struct EventScheduler {
    EventScheduler()
        : loop_{ static_cast<uv_loop_t *>( std::malloc( sizeof( uv_loop_t ) ) ) } {
        // Try to allocate main loop handle.

        // We not yet in loop thread. Calling this is safe.
        const auto ret = uv_loop_init( loop_ );

        // Loop thread initialization.
        thread_ = std::make_unique<std::thread>( [this]() -> void {
            // Main thread must explicitly notify this
            // thread to start event loop.
            std::unique_lock<std::mutex> lk{ m_ };
            cv_.wait( lk, [this]() -> bool {
                //
                return run_;
            } );

            // Start event loop.
            uv_run( loop_, UV_RUN_DEFAULT );

            log::info()( "uv loop closed!" );
        } );
    }

    // Not copyable not moveable "service-like" object.
    EventScheduler( const EventScheduler &other ) = delete;
    EventScheduler( EventScheduler &&other ) = delete;
    auto operator=( const EventScheduler &other ) -> EventScheduler & = delete;
    auto operator=( EventScheduler &&other ) -> EventScheduler & = delete;

    virtual ~EventScheduler() {
        // We already left loop thread.
        // De-initialize and free the resources associated with an event loop.
        uv_loop_close( loop_ );

        // Release loop pointer.
        std::free( loop_ );
    }

    auto run() -> void {
        if ( run_ ) {
            log::warning()( "can't do run(), context already active!" );
            return;
        }

        {
            std::lock_guard<std::mutex> _{ m_ };
            run_ = true;
        }
        cv_.notify_one();

        thread_->detach();
    }

    auto syncWait() -> void {
        if ( run_ ) {
            log::warning()( "can't do syncWait(), context already "
                            "active!" );
            return;
        }

        {
            std::lock_guard<std::mutex> _{ m_ };
            run_ = true;
        }
        cv_.notify_one();

        //
        thread_->join();
    }

protected:
    // Submit callback to event loop.
    // That async handle contain a pointer to specific data, that will be
    // used in callback and callback to perform action on event loop thread itself.
    auto schedule( void *payload, std::invocable<uv_async_t *> auto cb ) -> void {
        // Allocate uv_async_t handle. Will be deleted in close callback.
        const auto j = static_cast<uv_async_t *>( std::malloc( sizeof( uv_async_t ) ) );

        // Store payload pointer in async handle
        j->data = payload;

        // Initilaize async handle with specific payload and callback.
        // NOTE: this is (probably) not thread safe!
        uv_async_init( loop_, j, cb );

        // Submit job to event loop.
        // This thread safe (one and only thread safe call across
        // all lobuv API)
        uv_async_send( j );
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
};

}  // namespace tire::io
