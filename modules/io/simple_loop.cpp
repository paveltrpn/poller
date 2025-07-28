module;

#include <iostream>
#include <thread>
#include <uv.h>
#include <cstdlib>
#include <mutex>

export module io:simple_loop;

namespace poller::io {
/*
struct TimerHandle final {
    uint64_t timeout_{};
    uint64_t repeat_{};
    uv_timer_t handle_{};
    // uv_timer_cb type
    void ( *cb_ )( uv_timer_t *handle );
};

export struct SimpleLoop final {
    SimpleLoop() {
        loop_ = static_cast<uv_loop_t *>( malloc( sizeof( uv_loop_t ) ) );
        uv_loop_init( loop_ );

        thread_ = std::make_unique<std::thread>( [this]() {
            std::println( "SimpleLoop === start loop..." );

            uv_run( loop_, UV_RUN_DEFAULT );

            std::println( "SimpleLoop === loop stopped..." );
        } );
    }

    ~SimpleLoop() {
        std::println( "SimpleLoop === dtor..." );
        uv_idle_stop( &idle_ );
        uv_loop_close( loop_ );
        free( loop_ );
    }

    auto run() -> void {
        //
        thread_->join();
    }

    auto startIdle() -> void {
        uv_idle_init( loop_, &idle_ );
        uv_idle_start( &idle_, []( uv_idle_t *handle ) {} );
    }

    auto setCloseTimerAsync( long long msec ) -> void {
        closeTimer_.handle_.data = this;
        closeTimer_.timeout_ = msec;
        closeTimer_.repeat_ = 0;
        closeTimer_.cb_ = []( uv_timer_t *handle ) {
            const auto th = static_cast<SimpleLoop *>( handle->data );
            std::println( "close timer fires..." );
            uv_stop( th->loop_ );
        };

        r1_.data = &closeTimer_;

        uv_async_init( loop_, &r1_, []( uv_async_t *handle ) {
            //
            auto timerHandle = static_cast<TimerHandle *>( handle->data );
            uv_timer_init( handle->loop, &timerHandle->handle_ );
            uv_timer_start( &timerHandle->handle_, timerHandle->cb_,
                            timerHandle->timeout_, timerHandle->repeat_ );

            std::println( "timer started by async..." );
        } );
        uv_async_send( &r1_ );
    }

    auto printThreadInfo() -> void {
        r0_.data = this;
        mainTID_ = std::this_thread::get_id();

        uv_async_init( loop_, &r0_, []( uv_async_t *handle ) {
            const auto th = static_cast<SimpleLoop *>( handle->data );
            std::println( "main thread id: {}", th->mainTID_ );
            const auto ttId = std::this_thread::get_id();
            std::println( "loop thread id: {}", ttId );
        } );

        uv_async_send( &r0_ );
    }

private:
    std::mutex m_;
    std::unique_ptr<std::thread> thread_;
    std::thread::id mainTID_{};

    uv_loop_t *loop_{};

    uv_idle_t idle_{};
    TimerHandle closeTimer_{};

    uv_async_t r0_{};
    uv_async_t r1_{};
};
*/
}  // namespace poller::io
