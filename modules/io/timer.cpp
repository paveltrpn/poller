module;

#include <print>
#include <uv.h>

export module io:timer;
import :event_scheduler;

namespace poller::io {

export struct Timer final : EventScheduler {
private:
    uv_timer_t timer_;
};

}  // namespace poller::io
