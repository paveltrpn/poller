
module;

#include <format>
#include <coroutine>
#include <print>
#include <string>
#include <thread>
#include <chrono>

export module httpbin;

import poller;

namespace httpbin {

using namespace std::chrono_literals;

const std::string HTTPBIN_USERAGENT = "http://httpbin.org/user-agent";
const std::string HTTPBIN_IP = "http://httpbin.org/ip";
const std::string HTTPBIN_HEADERS = "http://httpbin.org/headers";
const std::string HTTPBIN_DELAY_2S = "http://httpbin.org/delay/2";

export struct HttpbinClient final : poller::Poller {
    HttpbinClient() = default;

    HttpbinClient( const HttpbinClient& other ) = delete;
    HttpbinClient( HttpbinClient&& other ) = delete;

    auto operator=( const HttpbinClient& other ) -> HttpbinClient& = delete;
    auto operator=( HttpbinClient&& other ) -> HttpbinClient& = delete;

    ~HttpbinClient() = default;

    auto run() -> void override {
        {
            auto req = poller::HttpRequest{};
            req.setUrl( HTTPBIN_USERAGENT );
            request( std::move( req ) );
        }

        {
            auto req = poller::HttpRequestGet{};
            req.setUrl( HTTPBIN_HEADERS )
                .setHeader( "First-Header", "value" )
                .setHeader( "Second-Header", "value" )
                .setHeader( "Third-Header", "value" )
                .bake();
            request( std::move( req ) );
        }

        {
            auto req = poller::HttpRequest{};
            req.setUrl( HTTPBIN_IP ).gentlyUseV2().bake();
            request( std::move( req ) );
        }

        {
            auto req = poller::HttpRequest{};
            req.setUrl( HTTPBIN_DELAY_2S ).setTimeout( 1 ).bake();
            request( std::move( req ) );
        }
    }

private:
    auto request( poller::HttpRequest req ) -> poller::Task<void> {
        auto resp = co_await requestAsync<void>( std::move( req ) );

        const auto [code, data, headers] = resp;

        // std::println( "response code: {}\ndata:\n{}\nheaders:\n{}", code, data,
        // headers );

        std::println( "response code: {}\ndata:\n{}\n", code, data );
    }
};

}  // namespace httpbin