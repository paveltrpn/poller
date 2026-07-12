
module;

#include <string>

#include <uv.h>

export module io:payload;

namespace poller::io {

export struct AsyncJobPayload {
    void *coro{};
};

export struct TimeoutCbPayload final : public AsyncJobPayload {
    uint64_t timeout{};
};

export struct FileIOCbPayload final : public AsyncJobPayload {
    std::string path{};
    size_t opentResult;
};

}  // namespace poller::io