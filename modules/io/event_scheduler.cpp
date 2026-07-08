
module;

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

            // Gracefull shutdown.
            uv_close( reinterpret_cast<uv_handle_t *>( &asyncWakeup_ ), nullptr );
            // Finish still running close callbacks
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

    void postTask( std::function<void( uv_loop_t * )> task ) {
        {
            std::lock_guard<std::mutex> lock( queueMutex_ );
            queue_.push( std::move( task ) );
        }
        // Будим event loop, если он сейчас внутри uv_run
        uv_async_send( &asyncWakeup_ );
        // Будим поток, если он сейчас спит на condition_variable (после завершения uv_run)
        cv_.notify_one();
    }

private:
    static auto asyncCallback( uv_async_t *handle ) -> void {
        auto *ctx = static_cast<EventScheduler *>( handle->data );
        std::queue<std::function<void( uv_loop_t * )>> local_queue;

        // Быстро забираем все задачи из общей очереди в локальную
        {
            std::lock_guard<std::mutex> lock( ctx->queueMutex_ );
            std::swap( local_queue, ctx->queue_ );
        }

        // Выполняем задачи (создаем handles, запускаем I/O)
        while ( !local_queue.empty() ) {
            auto task = std::move( local_queue.front() );
            local_queue.pop();
            task( ctx->loop_ );
        }
    }
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

    //
    uv_async_t asyncWakeup_{};

    // Loop thread.
    std::unique_ptr<std::thread> thread_;

    //
    std::condition_variable cv_;
    std::mutex m_;

    //
    std::queue<std::function<void( uv_loop_t * )>> queue_;
    std::mutex queueMutex_;

    //
    bool run_{ true };
};

}  // namespace poller::io
