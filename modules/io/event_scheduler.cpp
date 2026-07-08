
module;

#include <thread>
#include <mutex>
#include "uv.h"
#include <condition_variable>

export module io:event_scheduler;

import log;

namespace poller::io {

struct EventScheduler {
    EventScheduler( const EventScheduler &other ) = delete;
    EventScheduler( EventScheduler &&other ) = delete;

    EventScheduler()
        : loop_{ //
                 // Try to allocate main loop handle.
                 static_cast<uv_loop_t *>( std::malloc( sizeof( uv_loop_t ) ) ) } {
        // We not yet in loop thread. Calling this is safe.
        const auto ret = uv_loop_init( loop_ );

        // Loop thread initialization.
        thread_ = std::make_unique<std::thread>( [this]() -> void {
            // Start event loop.
            while ( run_ ) {
                auto more = uv_run( loop_, UV_RUN_DEFAULT );

                log::info()( "wait for jobs..." );

                // Main thread must explicitly notify this
                // thread to start event loop.
                if ( more == 0 ) {
                    std::unique_lock<std::mutex> lk{ m_ };
                    cv_.wait( lk, [this]() -> bool {
                        //
                        return !run_;
                    } );
                }
            }

            // Корректное завершение
            // uv_close( reinterpret_cast<uv_handle_t *>( &ctx->async_wakeup ), nullptr );
            // Доработать оставшиеся close callbacks
            uv_run( loop_, UV_RUN_DEFAULT );
            uv_loop_close( loop_ );

            log::info()( "loop finished..." );
        } );
    }

    auto operator=( const EventScheduler &other ) -> EventScheduler & = delete;
    auto operator=( EventScheduler &&other ) -> EventScheduler & = delete;

    virtual ~EventScheduler() {
        stop();

        // Release loop pointer.
        std::free( loop_ );
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

        cv_.notify_one();
    }

private:
    auto stop() -> void {
        {
            // std::lock_guard<std::mutex> _{ some_mutex };
            run_ = false;
        }

        cv_.notify_one();
        thread_->join();
    }

private:
    // Main and only uv loop handle.
    uv_loop_t *loop_{};

    // Loop thread.
    std::unique_ptr<std::thread> thread_;

    //
    std::condition_variable cv_;
    std::mutex m_;

    //
    bool run_{ true };
};

}  // namespace poller::io
