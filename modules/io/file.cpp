module;

#include <print>
#include <vector>
#include <memory>
#include <uv.h>

export module io:file;
import :event_scheduler;

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

export struct File final : EventScheduler {
private:
    std::vector<std::unique_ptr<ReadHandle>> readPool_{};
    std::vector<std::unique_ptr<WriteHandle>> writPool_{};
};

}  // namespace poller::io
