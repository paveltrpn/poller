
module;

#include <thread>
#include <functional>

#include "uv.h"

export module io:schedulerbase;

import log;
import poller_std;
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

private:
    // This static method executes inside event loop when uv_async_send()
    // is called.
    static auto asyncCallback( uv_async_t *handle ) -> void {
        auto *ctx = static_cast<SchedulerBase *>( handle->data );

        // Execute tasks. Every task create libuv asunchronius
        // job throgh handles.
        while ( !ctx->asyncJobQueue_.is_empty() ) {
            auto item = std::pair<std::function<void( uv_loop_t *, AsyncJobPayload * )>, AsyncJobPayload *>{};
            ctx->asyncJobQueue_.pop( item );
            auto [task, payload] = item;
            task( ctx->loop_, payload );
        }
    }

    auto stop() -> void {
        // Unref uv_async_t to release event loop.
        uv_unref( reinterpret_cast<uv_handle_t *>( &asyncWakeup_ ) );

        // Work out unfinished jobs.
        uv_async_send( &asyncWakeup_ );
    }

protected:
    //
    uv_async_t asyncWakeup_{};
    poller::locking_queue<std::pair<std::function<void( uv_loop_t *, AsyncJobPayload * )>, AsyncJobPayload *>>
      asyncJobQueue_{ 128 };

private:
    // Main and only uv loop handle.
    uv_loop_t *loop_{};

    // Loop thread.
    std::unique_ptr<std::thread> thread_{};
};

}  // namespace poller::io
