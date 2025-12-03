
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

export struct HttpbinClient final : poller::Poller {
    HttpbinClient() = default;

    HttpbinClient( const HttpbinClient& other ) = delete;
    HttpbinClient( HttpbinClient&& other ) = delete;

    auto operator=( const HttpbinClient& other ) -> HttpbinClient& = delete;
    auto operator=( HttpbinClient&& other ) -> HttpbinClient& = delete;

    ~HttpbinClient() = default;

    auto run() -> void override {
        requestAsync( HTTPBIN_USERAGENT );

        {
            auto req = poller::HttpRequest{};
            req.setUrl( HTTPBIN_USERAGENT );
            requestAsync( std::move( req ) );
        }

        {
            auto req = poller::HttpRequestGet{};
            req.setUrl( HTTPBIN_HEADERS )
                .setHeader( "First-Header", "value" )
                .setHeader( "Second-Header", "value" )
                .setHeader( "Third-Header", "value" )
                .bake();
            requestAsync( std::move( req ) );
        }

        {
            auto req = poller::HttpRequest{};
            req.setUrl( HTTPBIN_IP ).gentlyUseV2().bake();
            requestAsync( std::move( req ) );
        }

        submit();
    }

private:
    auto requestAsync( std::string rqst ) -> poller::Task<void> {
        auto resp = co_await requestAsyncVoid( rqst );

        const auto [code, data, headers] = resp;

        std::println( "response code: {}\ndata:\n{}\nheaders:\n{}", code, data,
                      headers );
    }

    auto requestAsync( poller::HttpRequest&& req ) -> poller::Task<void> {
        auto resp = co_await requestAsyncVoid( std::move( req ) );

        const auto [code, data, headers] = resp;

        std::println( "response code: {}\ndata:\n{}\nheaders:\n{}", code, data,
                      headers );
    }
};

}  // namespace httpbin