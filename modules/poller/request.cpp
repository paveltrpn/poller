module;

#include <string>
#include <numeric>
#include <span>
#include <curl/curl.h>

export module poller:request;

import :handle;

namespace poller {

auto urlEncode( CURL* curl, std::string_view url ) -> std::string {
    char* result =
        curl_easy_escape( curl, url.data(), static_cast<int>( url.size() ) );
    const std::string encoded{ result };
    curl_free( result );
    return encoded;
}

auto formattedFields(
    CURL* curl, std::span<const std::pair<std::string, std::string>> fields )
    -> std::string {
    std::string result;
    result.reserve( std::accumulate(
        fields.begin(), fields.end(), 0, []( size_t acc, const auto& pair ) {
            return acc + pair.first.size() + pair.second.size() + 2;
        } ) );

    for ( const auto& [key, value] : fields ) {
        result += urlEncode( curl, key );
        result += '=';
        result += urlEncode( curl, value );
        result += '&';
    }
    return result;
}

export struct HttpRequest {
    HttpRequest( const std::string& url, const std::string& userAgent ) {
        handle_.setopt<CURLOPT_URL>( url );
        handle_.setopt<CURLOPT_USERAGENT>( userAgent );
    };

    HttpRequest( const HttpRequest& other ) = delete;

    HttpRequest( HttpRequest&& other ) noexcept {
        //
        handle_ = std::move( other.handle_ );
    }

    ~HttpRequest() = default;

    auto operator=( const HttpRequest& other ) -> HttpRequest& = delete;

    auto operator=( HttpRequest&& other ) noexcept -> HttpRequest& {
        if ( this != &other ) {
            handle_ = std::move( other.handle_ );
        }
        return *this;
    }

    auto isValid() -> bool {
        //
        return handle_.isValid();
    }

    auto handle() -> Handle& {
        //
        return handle_;
    };

    operator CURL*() {
        //
        return handle_;
    };

    auto enableDebug() -> void {
        //
        handle_.enableDebug();
    }

private:
    Handle handle_;
};

export struct HttpRequestGet final : HttpRequest {
    HttpRequestGet( const std::string& url, const std::string& userAgent )
        : HttpRequest( url, userAgent ) {}
};

export struct HttpRequestPost final : HttpRequest {};

export struct HttpRequestDelete final : HttpRequest {};

export struct HttpRequestPatch final : HttpRequest {};

export struct HttpRequestPut final : HttpRequest {};

}  // namespace poller
