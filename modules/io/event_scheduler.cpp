module;

#include <iostream>
#include <thread>
#include <uv.h>
#include <cstdlib>
#include <mutex>

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

private:
    std::unique_ptr<std::thread> thread_;
    uv_loop_t *loop_{};
};

}  // namespace poller::io
