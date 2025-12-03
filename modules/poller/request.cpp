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
        fields.begin(), fields.end(), 0,
        []( size_t acc, const auto& pair ) -> size_t {
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
    HttpRequest() = default;

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

    auto setUrl( const std::string& value ) -> HttpRequest& {
        handle_.setopt<CURLOPT_URL>( value );
        return ( *this );
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

protected:
    Handle handle_;
};

export struct HttpRequestGet final : HttpRequest {
    HttpRequestGet()
        : HttpRequest() {
        handle_.setopt<CURLOPT_HTTPGET>( 1l );
    }
};

export struct HttpRequestPost final : HttpRequest {
    HttpRequestPost()
        : HttpRequest() {
        handle_.setopt<CURLOPT_POST>( 1l );
    }

    // Example value:
    // "name=value&anotherkey=anothervalue"
    auto setPostfields( const std::string& value ) -> HttpRequestPost& {
        handle_.setopt<CURLOPT_POSTFIELDS>( value );
        handle_.setopt<CURLOPT_POSTFIELDSIZE>(
            static_cast<long>( value.length() ) );

        return ( *this );
    }
};

export struct HttpRequestDelete final : HttpRequest {
    HttpRequestDelete()
        : HttpRequest() {
        handle_.setopt<CURLOPT_HTTPGET>( 1l );
    }
};

export struct HttpRequestPut final : HttpRequest {
    HttpRequestPut()
        : HttpRequest() {
        handle_.setopt<CURLOPT_HTTPGET>( 1l );
    }
};

}  // namespace poller
