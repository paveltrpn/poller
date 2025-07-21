module;

#include <print>

export module io:reader;
import :event_scheduler;

namespace poller::io {

export struct Reader final : EventScheduler {
private:
};

}  // namespace poller::io
