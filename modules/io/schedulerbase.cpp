
module;

#include <print>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>

#include "uv.h"

export module io:schedulerbase;

import log;

namespace poller::io {

struct SchedulerBase {
    SchedulerBase( const SchedulerBase &other ) = delete;
    SchedulerBase( SchedulerBase &&other ) = delete;

    SchedulerBase()
        : loop_{ // Allocate main loop handle.
                 static_cast<uv_loop_t *>( std::malloc( sizeof( uv_loop_t ) ) ) } {
        // Start worker thread.
        thread_ = std::make_unique<std::thread>( [this]() -> void {
            const auto ret = uv_loop_init( loop_ );

            // Initialized uv_async_t keep loop in polling phase (loop
            // is "sleep").
            uv_async_init( loop_, &asyncWakeup_, asyncCallback );
            asyncWakeup_.data = this;

            // Start event loop.
            auto more = uv_run( loop_, UV_RUN_DEFAULT );

            // Gracefull shutdown.
            uv_close( reinterpret_cast<uv_handle_t *>( &asyncWakeup_ ), nullptr );
            // Finish still running close callbacks.
            uv_run( loop_, UV_RUN_DEFAULT );
            uv_loop_close( loop_ );

            log::info()( "loop thread finished..." );
        } );
    }

    auto operator=( const SchedulerBase &other ) -> SchedulerBase & = delete;
    auto operator=( SchedulerBase &&other ) -> SchedulerBase & = delete;

    virtual ~SchedulerBase() {
        stop();

        thread_->join();

        // Release loop pointer.
        std::free( loop_ );

        log::info()( "closing app..." );
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

        // Wakeup event loop and process task queue.
        uv_async_send( &asyncWakeup_ );
    }

private:
    // This static method executes inside event loop when uv_async_send()
    // is called.
    static auto asyncCallback( uv_async_t *handle ) -> void {
        auto *ctx = static_cast<SchedulerBase *>( handle->data );
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
        // Unref uv_async_t to release event loop.
        uv_unref( reinterpret_cast<uv_handle_t *>( &asyncWakeup_ ) );
        // Work out unfinished jobs.
        uv_async_send( &asyncWakeup_ );
    }

private:
    // Main and only uv loop handle.
    uv_loop_t *loop_{};

    //
    uv_async_t asyncWakeup_{};

    // Loop thread.
    std::unique_ptr<std::thread> thread_{};

    //
    std::queue<std::pair<std::function<void( uv_loop_t *, void * )>, void *>> queue_{};
    std::mutex queueMutex_{};
};

}  // namespace poller::io
