module;

#include <print>

export module io:writer;
import :event_scheduler;

namespace poller::io {

export struct Writer final : EventScheduler {
private:
};

}  // namespace poller::io
