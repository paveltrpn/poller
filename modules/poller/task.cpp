
module;

#include <coroutine>
#include <exception>
#include <type_traits>
#include <concepts>
#include <functional>
#include <mutex>
#include <print>
#include <chrono>
#include <thread>

export module poller:task;

import :result;

using namespace std::chrono_literals;

namespace poller {

export template <typename T>
concept TaskParamter =
    std::is_same_v<T, void> || std::is_same_v<T, std::string> ||
    std::is_same_v<T, std::pair<int, std::string>>;

// Task class with get() method, block caller thread and
// and return value when ready.
export template <TaskParamter T>
struct Task {
    using value_type = T;

    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {
    public:
        auto get_return_object() -> Task {
            //
            return handle_type::from_promise( *this );
        };

        auto initial_suspend() noexcept -> std::suspend_never {
            //
            return {};
        }

        // suspend_always because we call coroutine destroy() which cause
        // UB if coroutine not in suspention point.
        auto final_suspend() noexcept -> std::suspend_always {
            {
                std::lock_guard<std::mutex> _{ m_ };
                ready_ = true;
            }
            cv_.notify_one();

            return {};
        }

        auto return_value( value_type value ) -> void {
            //
            payload_ = std::move( value );
        }

        auto unhandled_exception() -> void {
            //
            exception_ = std::current_exception();
        }

    public:
        value_type payload_;

        std::condition_variable cv_;
        std::mutex m_;
        bool ready_{};

        std::exception_ptr exception_{ nullptr };
    };

    Task( handle_type h )
        : handle_( h ) { /* noop */ }

    Task( Task&& t ) noexcept
        : handle_( t.handle_ ) {
        t.handle_ = nullptr;
    }

    auto operator=( Task&& other ) noexcept -> Task& {
        if ( std::addressof( other ) != this ) {
            if ( handle_ ) {
                handle_.destroy();
            }

            handle_ = other.handle_;
            other.handle_ = nullptr;
        }

        return *this;
    }

    // Move only.
    Task( const Task& ) = delete;
    auto operator=( const Task& ) -> Task& = delete;

    ~Task() {
        //
        detach();
    }

    auto detach() noexcept -> void {
        if ( empty() ) {
            return;
        }

        if ( handle_ ) {
            handle_.destroy();
            handle_ = nullptr;
        }
    }

    [[nodiscard]]
    auto empty() const noexcept -> bool {
        //
        return handle_ == nullptr;
    }

    explicit operator bool() const noexcept {
        //
        return !empty();
    }

    // Block caller thread until coroutine reach final_suspend()
    [[nodiscard]]
    auto get() -> value_type {
        std::unique_lock<std::mutex> lk{ handle_.promise().m_ };
        handle_.promise().cv_.wait( lk, [this]() -> bool {
            //
            return handle_.promise().ready_;
        } );

        return handle_.promise().payload_;
    }

private:
    handle_type handle_{ nullptr };
};

// Simple task class for void-returning coroutines.
export template <>
struct Task<void> {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {
    public:
        auto get_return_object() -> Task {
            //
            return {};
        };

        auto initial_suspend() noexcept {
            //
            return std::suspend_never{};
        }

        auto final_suspend() noexcept {
            //
            return std::suspend_never{};
        }

        auto return_void() -> void { /* noop */ }

        auto unhandled_exception() -> void {
            //
            exception_ = std::current_exception();
        }

    public:
        std::exception_ptr exception_{ nullptr };
    };
};

}  // namespace poller
