module;

#include <iostream>
#include <thread>
#include <uv.h>

export module io:event_scheduler;

namespace poller::io {

export struct EventScheduler final {

private:
    std::thread _thread;
};

}  // namespace poller::io
