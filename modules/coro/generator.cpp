
module;

#include <concepts>
#include <coroutine>
#include <exception>
#include <utility>

export module coro:generator;

namespace poller {

export {
    template <typename T>
    struct Generator final {
        struct promise_type;
        using handle_type = std::coroutine_handle<promise_type>;

        struct promise_type {
            // The result of that call will be returned to the caller when the
            // coroutine first suspends. Any exceptions thrown up to and including
            // this step propagate back to the caller, not placed in the promise
            Generator get_return_object() {
                return Generator( handle_type::from_promise( *this ) );
            }

            std::suspend_always initial_suspend() { return {}; }

            std::suspend_always final_suspend() noexcept { return {}; }

            void unhandled_exception() {
                exception_ = std::current_exception();
            }

            // Useful with "co_yield i;"  operator
            //
            // template <std::convertible_to<T> From>
            // std::suspend_always yield_value( From &&from ) {
            // value_ = std::forward<From>( from );
            // return {};
            // }

            // Useful with "co_await i;"  operator
            //
            template <std::convertible_to<T> From>
            std::suspend_always await_transform( From&& from ) {
                value_ = std::forward<From>( from );
                return {};
            }

            // Can be only one of two
            void return_void() {
                //
                // noop
            }

            // void return_value() {}

            T value_;
            std::exception_ptr exception_;
        };

        handle_type h_;

        Generator( handle_type h )
            : h_( h ) {}

        ~Generator() { h_.destroy(); }

        explicit operator bool() {
            fill();
            return !h_.done();
        }

        T operator()() {
            fill();
            full_ = false;
            return std::move( h_.promise().value_ );
        }

    private:
        void fill() {
            if ( !full_ ) {
                h_();  // resume, equal to h_.resume()
                if ( h_.promise().exception_ ) {
                    std::rethrow_exception( h_.promise().exception_ );
                }
                full_ = true;
            }
        }

    private:
        bool full_ = false;
    };
}

}  // namespace poller
