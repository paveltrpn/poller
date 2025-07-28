module;

#include <memory>
#include <iostream>
#include <thread>
#include <cstdlib>
#include <mutex>
#include <vector>

#include <uv.h>

export module io:event_scheduler;

namespace poller::io {

struct EventScheduler {
    EventScheduler() {
        loop_ = static_cast<uv_loop_t *>( malloc( sizeof( uv_loop_t ) ) );
        uv_loop_init( loop_ );

        thread_ = std::make_unique<std::thread>( [this]() {
            //
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
                           &item ) ) == 0;
            } );

        if ( it != pendingQueue_.cend() ) {
            return std::distance( pendingQueue_.begin(), it );
        } else {
            return -1;
        }
    }

    auto scheduleJob() -> uv_async_t * {
        auto j = std::make_unique<uv_async_t>();
        const auto tmp = j.get();
        pendingQueue_.push_back( std::move( j ) );
        return tmp;
    }

protected:
    uv_loop_t *loop_{};

private:
    std::unique_ptr<std::thread> thread_;

    std::vector<std::unique_ptr<uv_async_t>> pendingQueue_{};
};

}  // namespace poller::io
