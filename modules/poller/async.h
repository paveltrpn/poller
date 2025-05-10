
#pragma once

#include <coroutine>
#include <exception>
#include <mutex>
#include <print>

#include "poller.h"

namespace poller {

template <typename T>
struct Task;

template <>
struct Task<void> {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {
        std::exception_ptr exception_{ nullptr };

        Task get_return_object() { return {}; };

        auto initial_suspend() noexcept { return std::suspend_never{}; }
        auto final_suspend() noexcept { return std::suspend_never{}; }

        void return_void() {}

        void unhandled_exception() { exception_ = std::current_exception(); }
    };
};

namespace __detail__ {
// Global sync primitives, uses for suspend main thread when
// call Task<Result>::get() if request not ready. Single
// instance for all available coroutines.
static std::condition_variable cv_;
static std::mutex mtx_;
}  // namespace __detail__

template <>
struct Task<Result> {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {
        std::exception_ptr exception_{ nullptr };
        Result payload_;

        Task get_return_object() { return handle_type::from_promise( *this ); };

        void return_value( Result value ) { payload_ = value; }

        auto initial_suspend() noexcept { return std::suspend_never{}; }
        auto final_suspend() noexcept {
            __detail__::cv_.notify_all();
            return std::suspend_always{};
        }

        void unhandled_exception() { exception_ = std::current_exception(); }
    };

    Task( handle_type h )
        : handle_( h ) {}

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

    ~Task() { detach(); }

    void detach() noexcept {
        if ( empty() ) {
            return;
        }

        if ( handle_ ) {
            handle_.destroy();
            handle_ = nullptr;
        }
    }

    constexpr bool empty() const noexcept { return handle_ == nullptr; }
    constexpr explicit operator bool() const noexcept { return !empty(); }

    Result get() {
        // block caller thread until coroutine reach final_suspend()
        std::unique_lock<std::mutex> lk{ __detail__::mtx_ };
        __detail__::cv_.wait( lk, [this]() { return handle_.done(); } );

        return handle_.promise().payload_;
    }

private:
    handle_type handle_{ nullptr };
};

template <typename T, typename U>
struct RequestAwaitable final {
    RequestAwaitable( Poller& client, T request )
        : client_( client )
        , request_( std::move( request ) ){};

    // HTTP request always NOT ready immedieateley!
    bool await_ready() const noexcept { return false; }

    // can be void, bool, coroutine_handle<>
    auto await_suspend(
        std::coroutine_handle<typename U::promise_type> handle ) noexcept {
        client_.performRequest( std::move( request_ ),
                                [handle, this]( Result res ) {
                                    result_ = std::move( res );
                                    handle.resume();
                                } );
    }

    Result await_resume() const noexcept { return std::move( result_ ); }

    Poller& client_;
    T request_;
    Result result_;
};

}  // namespace poller
