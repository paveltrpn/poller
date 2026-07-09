
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
import :payload;

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
            uv_async_init( loop_, &timeoutAsyncWakeup_, timeoutAsyncCallback );
            timeoutAsyncWakeup_.data = this;

            uv_async_init( loop_, &fileIOAsyncWakeup_, fileioAsyncCallback );
            fileIOAsyncWakeup_.data = this;

            // Start event loop.
            auto more = uv_run( loop_, UV_RUN_DEFAULT );

            // Gracefull shutdown.
            uv_close( reinterpret_cast<uv_handle_t *>( &timeoutAsyncWakeup_ ), nullptr );
            uv_close( reinterpret_cast<uv_handle_t *>( &fileIOAsyncWakeup_ ), nullptr );
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
    void scheduleTimeout( std::function<void( uv_loop_t *, TimeoutCbPayload * )> task, TimeoutCbPayload *p ) {
        {
            std::lock_guard<std::mutex> lock( timeoutQueueMutex_ );
            timeoutQueue_.emplace( std::move( task ), p );
        }

        // Wakeup event loop and process task queue.
        uv_async_send( &timeoutAsyncWakeup_ );
    }

    void scheduleFileIO( std::function<void( uv_loop_t *, FileIOCbPayload * )> task, FileIOCbPayload *p ) {
        {
            std::lock_guard<std::mutex> lock( fileIOQueueMutex_ );
            fileIOQueue_.emplace( std::move( task ), p );
        }

        // Wakeup event loop and process task queue.
        uv_async_send( &fileIOAsyncWakeup_ );
    }

private:
    // This static method executes inside event loop when uv_async_send()
    // is called.
    static auto timeoutAsyncCallback( uv_async_t *handle ) -> void {
        auto *ctx = static_cast<SchedulerBase *>( handle->data );
        std::queue<std::pair<std::function<void( uv_loop_t *, TimeoutCbPayload * )>, TimeoutCbPayload *>> local_queue;

        // Move tasks from shared queue into local queue to
        // release the mutex as fast as we can.
        {
            std::lock_guard<std::mutex> lock( ctx->timeoutQueueMutex_ );
            std::swap( local_queue, ctx->timeoutQueue_ );
        }

        // Execute tasks. Every task create libuv asunchronius
        // job throgh handles.
        while ( !local_queue.empty() ) {
            auto [task, payload] = std::move( local_queue.front() );
            local_queue.pop();
            task( ctx->loop_, payload );
        }
    }

    static auto fileioAsyncCallback( uv_async_t *handle ) -> void {
        auto *ctx = static_cast<SchedulerBase *>( handle->data );
        std::queue<std::pair<std::function<void( uv_loop_t *, FileIOCbPayload * )>, FileIOCbPayload *>> local_queue;

        {
            std::lock_guard<std::mutex> lock( ctx->fileIOQueueMutex_ );
            std::swap( local_queue, ctx->fileIOQueue_ );
        }

        while ( !local_queue.empty() ) {
            auto [task, payload] = std::move( local_queue.front() );
            local_queue.pop();
            task( ctx->loop_, payload );
        }
    }

    auto stop() -> void {
        // Unref uv_async_t to release event loop.
        uv_unref( reinterpret_cast<uv_handle_t *>( &timeoutAsyncWakeup_ ) );
        uv_unref( reinterpret_cast<uv_handle_t *>( &fileIOAsyncWakeup_ ) );

        // Work out unfinished jobs.
        uv_async_send( &timeoutAsyncWakeup_ );
        uv_async_send( &fileIOAsyncWakeup_ );
    }

private:
    // Main and only uv loop handle.
    uv_loop_t *loop_{};

    // Loop thread.
    std::unique_ptr<std::thread> thread_{};

    //
    uv_async_t timeoutAsyncWakeup_{};
    std::queue<std::pair<std::function<void( uv_loop_t *, TimeoutCbPayload * )>, TimeoutCbPayload *>> timeoutQueue_{};
    std::mutex timeoutQueueMutex_{};

    //
    uv_async_t fileIOAsyncWakeup_{};
    std::queue<std::pair<std::function<void( uv_loop_t *, FileIOCbPayload * )>, FileIOCbPayload *>> fileIOQueue_{};
    std::mutex fileIOQueueMutex_{};
};

}  // namespace poller::io
