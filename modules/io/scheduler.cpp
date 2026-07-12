
module;

#include <string>
#include <cstdlib>
#include <functional>

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

private:
    // Submit callback to event loop.
    void scheduleTimeout( std::function<void( uv_loop_t *, TimeoutCbPayload * )> task, TimeoutCbPayload *p ) {
        timeoutQueue_.push( std::make_pair( std::move( task ), p ) );

        // Wakeup event loop and process task queue.
        uv_async_send( &timeoutAsyncWakeup_ );
    }

    void scheduleFileIO( std::function<void( uv_loop_t *, FileIOCbPayload * )> task, FileIOCbPayload *p ) {
        fileIOQueue_.push( std::make_pair( std::move( task ), p ) );

        // Wakeup event loop and process task queue.
        uv_async_send( &fileIOAsyncWakeup_ );
    }
};

}  // namespace poller::io
