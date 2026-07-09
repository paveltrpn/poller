
module;

#include <string>
#include <cstdlib>
#include <coroutine>

#include <uv.h>

export module io:scheduler;

import :schedulerbase;
import :async;

namespace poller::io {

export template <typename T>
struct TimeoutAwaitable;

export template <typename T>
struct FileIOAwaitable;

export struct Scheduler final : SchedulerBase {
    template <typename T>
    friend struct TimeoutAwaitable;

    template <typename T>
    friend struct FileIOAwaitable;

public:
    // Fire once by timeout.
    auto timeout( uint64_t timeout ) -> TimeoutAwaitable<Task<void>>;

    //
    auto openFile( const std::string &path ) -> FileIOAwaitable<Task<void>>;
};

}  // namespace poller::io
