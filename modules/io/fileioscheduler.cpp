
module;

#include <cstdlib>
#include <coroutine>

#include <uv.h>

export module io:fileio_scheduler;

import :event_scheduler;
import :async;

namespace poller::io {

struct ReadHandle {
    uv_fs_t open_;
    uv_fs_t read_;
    uv_fs_t close_;
    uv_buf_t buf_;
};

struct WriteHandle {
    uv_fs_t open_;
    uv_fs_t write_;
    uv_fs_t close_;
    uv_buf_t buf_;
};

export struct FileIOSheduler final : EventScheduler {
public:
};

}  // namespace poller::io