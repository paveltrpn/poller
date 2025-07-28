module;

#include <print>

export module io:file;
import :event_scheduler;

namespace poller::io {

export struct File final : EventScheduler {
private:
};

}  // namespace poller::io
