
module;

#include <coroutine>
#include <exception>
#include <functional>
#include <mutex>
#include <print>

export module poller:async;

import :request;

namespace poller {

export struct Result {
    int code;
    std::string data;
};

export using CallbackFn = std::function<void( Result result )>;

export struct Request {
    CallbackFn callback;
    std::string buffer;
};

export template <typename T>
struct Task;

export template <>
struct Task<void> {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {
    public:
        Task get_return_object() {
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

namespace __detail__ {
// Global sync primitives, uses for suspend main thread when
// call Task<Result>::get() if request not ready. Single
// instance for all available coroutines.
static std::condition_variable cv_;
static std::mutex mtx_;
}  // namespace __detail__

export template <>
struct Task<Result> {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {
        std::exception_ptr exception_{ nullptr };
        Result payload_;

        Task get_return_object() {
            //
            return handle_type::from_promise( *this );
        };

        auto return_value( Result value ) -> void {
            // /
            payload_ = value;
        }

        auto initial_suspend() noexcept -> std::suspend_never {
            //
            return {};
        }

        auto final_suspend() noexcept -> std::suspend_always {
            __detail__::cv_.notify_all();
            return {};
        }

        auto unhandled_exception() -> void {
            //
            exception_ = std::current_exception();
        }
    };

    Task( handle_type h )
        : handle_( h ) { /* noop */ }

    Task( Task&& t ) noexcept
        : handle_( t.handle_ ) {
        t.handle_ = nullptr;
    }

    Task& operator=( Task&& other ) noexcept {
        if ( std::addressof( other ) != this ) {
            if ( handle_ ) {
                handle_.destroy();
            }

            handle_ = other.handle_;
            other.handle_ = nullptr;
        }

        return *this;
    }

    Task( const Task& ) = delete;
    Task& operator=( const Task& ) = delete;

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

    constexpr auto empty() const noexcept -> bool {
        //
        return handle_ == nullptr;
    }

    constexpr explicit operator bool() const noexcept {
        //
        return !empty();
    }

    Result get() {
        // block caller thread until coroutine reach final_suspend()
        std::unique_lock<std::mutex> lk{ __detail__::mtx_ };
        __detail__::cv_.wait( lk, [this]() { return handle_.done(); } );

        return handle_.promise().payload_;
    }

private:
    handle_type handle_{ nullptr };
};

}  // namespace poller
