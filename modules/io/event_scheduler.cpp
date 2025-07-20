module;

#include <iostream>
#include <thread>
#include <uv.h>
#include <cstdlib>
#include <mutex>

export module io:event_scheduler;

namespace poller::io {

export struct EventScheduler final {
    EventScheduler() {
        loop_ = static_cast<uv_loop_t *>( malloc( sizeof( uv_loop_t ) ) );
        uv_loop_init( loop_ );

        thread_ = std::make_unique<std::thread>( [this]() {
            std::println( "EventScheduler === start loop..." );

            uv_run( loop_, UV_RUN_DEFAULT );

            std::println( "EventScheduler === loop stopped..." );
        } );
    }

    ~EventScheduler() {
        std::println( "EventScheduler === dtor..." );
        uv_idle_stop( &idle_ );
        uv_timer_stop( &timer_ );
        uv_loop_close( loop_ );
        free( loop_ );
    }

    auto run() -> void {
        //
        thread_->join();
    }

    auto passidle() -> void {
        uv_idle_init( loop_, &idle_ );
        uv_idle_start( &idle_, []( uv_idle_t *handle ) {} );
    }

    auto passTimer( long long msec ) -> void {
        uv_timer_init( loop_, &timer_ );
        uv_timer_start(
            &timer_,
            []( uv_timer_t *handle ) {
                //
                std::println( "timer cb..." );
            },
            1000, 2000 );
    }

private:
    std::mutex m_;

    uv_loop_t *loop_{};
    uv_idle_t idle_{};

    uv_timer_t timer_{};

    std::unique_ptr<std::thread> thread_;
};

}  // namespace poller::io
