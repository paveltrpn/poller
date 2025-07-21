module;

#include <print>
#include <uv.h>

export module io:timer;
import :event_scheduler;

namespace poller::io {

export struct Timer final : EventScheduler {
private:
};

}  // namespace poller::io
