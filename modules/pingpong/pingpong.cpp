module;

#include <print>
#include <coroutine>
#include <exception>

export module pingpong;

namespace poller {

export struct PingPongCoro final {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {
    public:
        PingPongCoro get_return_object() {
            //
            return handle_type::from_promise( *this );
        };

        auto initial_suspend() noexcept {
            //
            return std::suspend_always{};
        }

        auto final_suspend() noexcept {
            // NOTE: this couroutine type explicitly calls
            // coroutine_handle::destroy() in destructor. This method can be
            // called only on suspended coroutine! Hence we always suspend
            // at the end of execution.
            return std::suspend_always{};
        }

        auto yield_value( long long value ) -> std::suspend_always {
            value_ = value;
            return {};
        }

        auto return_void() -> void { /* noop */ }

        auto unhandled_exception() -> void {
            //
            exception_ = std::current_exception();
        }

    public:
        std::exception_ptr exception_{ nullptr };
        long long value_{};
    };

    PingPongCoro( handle_type h )
        : handle_( h ) { /* noop */ }

    PingPongCoro( PingPongCoro&& t ) noexcept
        : handle_( t.handle_ ) {
        t.handle_ = nullptr;
    }

    // Move only.
    PingPongCoro( const PingPongCoro& ) = delete;
    PingPongCoro& operator=( const PingPongCoro& ) = delete;

    PingPongCoro& operator=( PingPongCoro&& other ) noexcept {
        if ( std::addressof( other ) != this ) {
            if ( handle_ ) {
                handle_.destroy();
            }

            handle_ = other.handle_;
            other.handle_ = nullptr;
        }

        return *this;
    }

    ~PingPongCoro() {
        //
        detach();
    }

    constexpr auto empty() const noexcept -> bool {
        //
        return handle_ == nullptr;
    }

    constexpr explicit operator bool() const noexcept {
        //
        return !empty();
    }

    auto detach() noexcept -> void {
        if ( empty() ) {
            return;
        }

        if ( handle_ ) {
            std::println( "ping pong coro destroy" );

            // Can be called only on suspended coroutine!
            handle_.destroy();

            handle_ = nullptr;
        }
    }

    auto resume() {
        //
        handle_.resume();
    }

private:
    handle_type handle_{ nullptr };
};

}  // namespace poller
