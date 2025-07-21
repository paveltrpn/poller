module;

#include <algorithm>
#include <print>
#include <array>
#include <iterator>
#include <uv.h>

export module io:timer;
import :event_scheduler;

namespace poller::io {

export struct Timer final : EventScheduler {
private:
    // Find first active uv_timer_t handle that can be used
    // to perform operation.
    // NOTE: just liniera search.
    auto findActive() -> size_t {
        const auto it =
            std::find_if( pool_.cbegin(), pool_.cend(), []( const auto &item ) {
                return uv_is_active( reinterpret_cast<const uv_handle_t *>(
                           &item ) ) != 0;
            } );

        if ( it != pool_.cend() ) {
            return std::distance( pool_.cbegin(), it );
        } else {
            return -1;
        }
    }

private:
#define TIMER_POOL_SIZE 8
    std::array<uv_timer_t, TIMER_POOL_SIZE> pool_;
};

}  // namespace poller::io
