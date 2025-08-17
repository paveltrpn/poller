module;

#include <string>
#include <numeric>
#include <span>
#include <curl/curl.h>

export module poller:request;

import :handle;

namespace poller {

export std::string urlEncode( CURL* curl, std::string_view url ) {
    char* result =
        curl_easy_escape( curl, url.data(), static_cast<int>( url.size() ) );
    const std::string encoded{ result };
    curl_free( result );
    return encoded;
}

export std::string formattedFields(
    CURL* curl, std::span<const std::pair<std::string, std::string>> fields ) {
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
    HttpRequest& operator=( const HttpRequest& other ) = delete;

    HttpRequest( HttpRequest&& other ) {
        //
        handle_ = std::move( other.handle_ );
    }

    HttpRequest& operator=( HttpRequest&& other ) {
        if ( this != &other ) {
            handle_ = std::move( other.handle_ );
        }
        return *this;
    }

    bool isValid() {
        //
        return handle_.isValid();
    }

    Handle& handle() {
        //
        return handle_;
    };

    operator CURL*() {
        //
        return handle_;
    };

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
