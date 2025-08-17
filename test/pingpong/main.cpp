
#include <print>
#include <coroutine>
#include <vector>
#include <thread>
#include <chrono>

import pingpong;

using namespace std::chrono_literals;

enum class Plyers { PING, PONG };

std::vector<poller::PingPongCoro*> players;

long long count{};

std::suspend_always operator co_await( Plyers player ) {
    players[static_cast<size_t>( player )]->resume();
    return {};
}

auto ping() -> poller::PingPongCoro {
    for ( ;; ) {
        ++count;

        std::this_thread::sleep_for( 250ms );

        std::println( "PING at {}", count );

        co_await Plyers::PONG;
    }
}

auto pong() -> poller::PingPongCoro {
    long long j{};

    for ( ;; ) {
        ++j;
        ++count;

        std::this_thread::sleep_for( 500ms );

        std::println( "PONG at {}", count );

        co_await Plyers::PING;
    }
}

int main( int argc, char** argv ) {
    auto pi = ping();
    players.push_back( &pi );

    auto po = pong();
    players.push_back( &po );

    pi.resume();

    return 0;
}
