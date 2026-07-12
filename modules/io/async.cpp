
module;

#include <string>
#include <coroutine>
#include <exception>
#include <uv.h>

export module io:async;

import log;

namespace poller::io {

export template <typename T>
struct Task;

export template <>
struct Task<void> {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {
        auto get_return_object() -> Task {
            //
            return Task<void>{ handle_type::from_promise( *this ) };
        };

        auto initial_suspend() noexcept {
            //
            return std::suspend_never{};
        }

        auto final_suspend() noexcept {
            //
            return std::suspend_never{};
        }

        auto return_void() -> void {
            //
        }

        auto unhandled_exception() -> void {
            //
            exception_ = std::current_exception();
        }

        std::exception_ptr exception_{ nullptr };
        bool scheduleDestroy_{ false };
    };

    explicit Task( handle_type h )
        : handle_( h ) {}

    Task() = default;

    Task( Task &&t ) noexcept
        : handle_( t.handle_ ) {
        t.handle_ = nullptr;
    }

    auto operator=( Task &&other ) noexcept -> Task & {
        if ( std::addressof( other ) != this ) {
            // Destroy this handle, but it allowed only when
            // coroutine is suspended.
            if ( handle_ ) {
                handle_.destroy();
            }

            handle_ = other.handle_;
            other.handle_ = nullptr;
        }

        return *this;
    }

    Task( const Task & ) = delete;
    auto operator=( const Task & ) -> Task & = delete;

    ~Task() = default;

    auto detach() noexcept -> void {
        if ( empty() ) {
            return;
        }

        if ( handle_ ) {
            handle_.destroy();
            handle_ = nullptr;
        }
    }

    [[nodiscard]] constexpr auto empty() const noexcept -> bool {
        //
        return handle_ == nullptr;
    }

    constexpr explicit operator bool() const noexcept {
        //
        return !empty();
    }

    auto scheduleDestroy() -> void {
        //
        handle_.promise().scheduleDestroy_ = true;
    };

private:
    handle_type handle_{ nullptr };
};

}  // namespace poller::io
