
#include <print>
#include <coroutine>
#include <vector>
#include <thread>
#include <chrono>

import coro;

using namespace std::chrono_literals;

enum class Plyers { PING, PONG };

std::vector<poller::PingPongCoro*> players;

long long count{};

std::suspend_always operator co_await( Plyers player ) {
    std::println( "=== {}", reinterpret_cast<uintptr_t>(
                                players[static_cast<size_t>( player )] ) );
    players[static_cast<size_t>( player )]->resume();
    return {};
}

struct PingPongAwaitable final {
    explicit PingPongAwaitable( Plyers p )
        : player_{ p } {}

    auto await_ready() -> bool {
        //
        return false;
    }

    auto await_suspend( std::coroutine_handle<> handle ) -> void {
        std::this_thread::sleep_for( 100ms );
        players[static_cast<size_t>( player_ )]->resume();
    }

    auto await_resume() -> void {
        //
    }

private:
    Plyers player_;
};

auto ping() -> poller::PingPongCoro {
    long long local{};

    for ( ;; ) {
        ++local;
        ++count;

        std::this_thread::sleep_for( 250ms );

        std::println( "PING at count {}, with local {}", count, local );

        //co_await Plyers::PONG;
        co_await PingPongAwaitable{ Plyers::PONG };

        if ( local > 2 ) break;
    }

    std::println( "PING ended" );

    // Just last step of another player.
    co_await PingPongAwaitable{ Plyers::PONG };
}

auto pong() -> poller::PingPongCoro {
    long long local{};

    for ( ;; ) {
        ++local;
        ++count;

        std::this_thread::sleep_for( 500ms );

        std::println( "PONG at count {}, with local {}", count, local );

        //co_await Plyers::PING;
        co_await PingPongAwaitable{ Plyers::PING };

        if ( local > 2 ) break;
    }

    std::println( "PONG ended" );

    // Just last step of another player.
    co_await PingPongAwaitable{ Plyers::PING };
}

int main( int argc, char** argv ) {
    auto pi = ping();
    players.push_back( &pi );

    auto po = pong();
    players.push_back( &po );

    pi.resume();

    return 0;
}
