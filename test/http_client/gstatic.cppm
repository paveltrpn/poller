module;

#include <format>
#include <coroutine>
#include <print>
#include <string>
#include <thread>
#include <chrono>

export module gstatic;

import poller;
import coro;

namespace gstatic {

using namespace std::chrono_literals;

const std::string GSTATIC_GENERATE404 = "http://www.gstatic.com/generate_204";

export struct GstaticClient final {
    GstaticClient() = default;

    GstaticClient( const GstaticClient& other ) = delete;
    GstaticClient( GstaticClient&& other ) = delete;

    auto operator=( const GstaticClient& other ) -> GstaticClient& = delete;
    auto operator=( GstaticClient&& other ) -> GstaticClient& = delete;

    ~GstaticClient() = default;

    auto run() -> void {
        //
        requestAsync( GSTATIC_GENERATE404 );

        client_.submit();
    }

private:
    auto requestAsync( std::string rqst ) -> poller::Task<void> {
        auto resp = co_await client_.requestAsyncVoid( rqst );
        std::println( "ready {} - {}", resp.code, resp.data );
    }

private:
    poller::Poller client_{};
};

}  // namespace gstatic
