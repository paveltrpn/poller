
module;

#include <string>

#include <uv.h>

export module io:payload;

namespace poller::io {

export struct TimeoutCbPayload {
    void *coro{};
    uint64_t timeout{};
};

export struct FileIOCbPayload {
    void *coro;
    std::string path{};
    size_t opentResult;
};

}  // namespace poller::io