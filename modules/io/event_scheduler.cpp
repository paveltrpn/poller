module;

#include <iostream>
#include <thread>
#include <uv.h>
#include <cstdlib>

export module io:event_scheduler;

namespace poller::io {

export struct EventScheduler final {
    EventScheduler() {
        loop_ = static_cast<uv_loop_t *>( malloc( sizeof( uv_loop_t ) ) );
    }

    ~EventScheduler() {
        //
        free( loop_ );
    }

private:
    uv_loop_t *loop_{};
    std::thread thread_;
};

}  // namespace poller::io
