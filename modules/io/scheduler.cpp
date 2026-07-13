
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

export template <typename T>
struct FilesystemWatchAwaitable;

export struct Scheduler final : SchedulerBase {
    template <typename T>
    friend struct TimeoutAwaitable;

    template <typename T>
    friend struct FileIOAwaitable;

    template <typename T>
    friend struct FilesystemWatchAwaitable;

public:
    // Fire once by timeout.
    auto timeout( uint64_t timeout ) -> TimeoutAwaitable<Task<void>>;

    //
    auto openFile( const std::string &path ) -> FileIOAwaitable<Task<void>>;

    //
    auto watchFile( const std::string &path ) -> FilesystemWatchAwaitable<Task<void>>;

private:
    // Submit callback to event loop.
    void schedule( std::function<void( uv_loop_t *, AsyncJobPayload * )> task, AsyncJobPayload *p ) {
        asyncJobQueue_.push( std::make_pair( std::move( task ), p ) );

        // Wakeup event loop and process task queue.
        uv_async_send( &asyncWakeup_ );
    }
};

}  // namespace poller::io
