
module;

#include <print>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>

#include "uv.h"

export module io:event_scheduler;

import log;

namespace poller::io {

struct EventScheduler {
    EventScheduler( const EventScheduler &other ) = delete;
    EventScheduler( EventScheduler &&other ) = delete;

    EventScheduler()
        : loop_{ // Allocate main loop handle.
                 static_cast<uv_loop_t *>( std::malloc( sizeof( uv_loop_t ) ) ) } {
        // Start worker thread.
        thread_ = std::make_unique<std::thread>( [this]() -> void {
            const auto ret = uv_loop_init( loop_ );

            uv_async_init( loop_, &asyncWakeup_, asyncCallback );
            asyncWakeup_.data = this;

            // Start event loop.
            while ( run_ ) {
                std::println( "start loop thread" );

                auto more = uv_run( loop_, UV_RUN_DEFAULT );

                log::info()( "wait for jobs... {}", more );

                // Main thread must explicitly notify this
                // thread to start event loop.
                if ( more == 0 ) {
                    std::unique_lock<std::mutex> lk{ m_ };
                    cv_.wait( lk, [this]() -> bool {
                        //
                        return !queue_.empty() || !run_;
                    } );
                }
            }

            // Gracefull shutdown.
            uv_close( reinterpret_cast<uv_handle_t *>( &asyncWakeup_ ), nullptr );
            // Finish still running close callbacks.
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
    void schedule( std::function<void( uv_loop_t *, void * )> task, void *coro ) {
        {
            std::lock_guard<std::mutex> lock( queueMutex_ );
            queue_.emplace( std::move( task ), coro );
        }
        // Будим event loop, если он сейчас внутри uv_run
        uv_async_send( &asyncWakeup_ );
        // Будим поток, если он сейчас спит на condition_variable (после завершения uv_run)
        cv_.notify_one();
    }

private:
    // This static method executes inside event loop when uv_async_send()
    // is called.
    static auto asyncCallback( uv_async_t *handle ) -> void {
        auto *ctx = static_cast<EventScheduler *>( handle->data );
        std::queue<std::pair<std::function<void( uv_loop_t *, void * )>, void *>> local_queue;

        // Move tasks from shared queue into local queue to
        // release the mutex as fast as we can.
        {
            std::lock_guard<std::mutex> lock( ctx->queueMutex_ );
            std::swap( local_queue, ctx->queue_ );
        }

        // Execute tasks. Every task create libuv asunchronius
        // job throgh handles.
        while ( !local_queue.empty() ) {
            auto [task, coro] = std::move( local_queue.front() );
            local_queue.pop();
            task( ctx->loop_, coro );
        }
    }

    auto stop() -> void {
        {
            std::lock_guard<std::mutex> _( queueMutex_ );
            run_ = false;
        }

        cv_.notify_one();
        thread_->join();
    }

private:
    // Main and only uv loop handle.
    uv_loop_t *loop_{};

    //
    uv_async_t asyncWakeup_{};

    // Loop thread.
    std::unique_ptr<std::thread> thread_;

    //
    std::condition_variable cv_;
    std::mutex m_;

    //
    std::queue<std::pair<std::function<void( uv_loop_t *, void * )>, void *>> queue_;
    std::mutex queueMutex_;

    //
    bool run_{ true };
};

}  // namespace poller::io
