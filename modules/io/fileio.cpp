
module;

#include <string>
#include <cstdlib>
#include <coroutine>

#include <uv.h>

export module io:fileio;

import :scheduler;
import :async;

namespace poller::io {

export template <typename T>
struct FileIOAwaitable final {
    FileIOAwaitable( Scheduler &context, std::string path )
        : context_( context )
        , path_{ std::move( path ) } {};

    [[nodiscard]]
    auto await_ready() const noexcept -> bool {
        //
        return false;
    }

    auto await_suspend( std::coroutine_handle<typename T::promise_type> coroHandle ) noexcept -> void {
        //
    }

    auto await_resume() const noexcept -> void {
        //
    }

    Scheduler &context_;
    std::string path_{};
};

auto Scheduler::openFile( const std::string &path ) -> FileIOAwaitable<Task<void>> {
    //
    return FileIOAwaitable<Task<void>>{ /* Context */ *this, path };
};

}  // namespace poller::io
