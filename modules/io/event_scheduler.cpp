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

namespace poller::io {

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

            uv_run( loop_, UV_RUN_DEFAULT );
        } );
    }

    EventScheduler( const EventScheduler &other ) = delete;
    EventScheduler( EventScheduler &&other ) = delete;
    EventScheduler &operator=( const EventScheduler &other ) = delete;
    EventScheduler &operator=( EventScheduler &&other ) = delete;

    virtual ~EventScheduler() {
        uv_loop_close( loop_ );
        free( loop_ );
    }

    auto run() -> void {
        {
            std::lock_guard<std::mutex> _{ m_ };
            run_ = true;
        }
        cv_.notify_one();
    }

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

    // Add new async handle in queue.
    auto scheduleJob() -> uv_async_t * {
        auto j = std::make_shared<uv_async_t>();
        const auto tmp = j.get();
        pendingQueue_.push_back( std::move( j ) );
        return tmp;
    }

protected:
    uv_loop_t *loop_{};

private:
    std::unique_ptr<std::thread> thread_;
    std::condition_variable cv_;
    std::mutex m_;
    bool run_{ false };

    std::vector<std::shared_ptr<uv_async_t>> pendingQueue_{};
};

}  // namespace poller::io
