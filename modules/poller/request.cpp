module;

#include <string>
#include <numeric>
#include <span>
#include <format>

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
            headers_ = other.headers_;
            other.headers_ = nullptr;
        }
        return *this;
    }

    auto setUrl( const std::string& value ) -> HttpRequest& {
        handle_.setopt<CURLOPT_URL>( value );
        return ( *this );
    }

    auto setHeader( const std::string& name, const std::string& value )
        -> HttpRequest& {
        const auto headerString = std::format( "{}: {}", name, value );

        headers_ = curl_slist_append( headers_, headerString.data() );

        return ( *this );
    }

    auto forceUseV2() -> HttpRequest& {
        // This option requires prior knowledge that the server
        // supports HTTP/2 directly, without an HTTP/1.1 Upgrade. If the
        // server does not support HTTP/2 in this manner, the
        // connection will likely fail.
        handle_.setopt<CURLOPT_HTTP_VERSION>(
            CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE );

        return ( *this );
    }

    auto gentlyUseV2() -> HttpRequest& {
        // This option tells libcurl to attempt to
        // use HTTP/2 over TLS, with a fallback
        // to HTTP/1.1 if negotiation fails.
        handle_.setopt<CURLOPT_HTTP_VERSION>( CURL_HTTP_VERSION_2TLS );

        return ( *this );
    }

    auto bake() -> void {
        if ( headers_ != nullptr ) {
            handle_.setopt<CURLOPT_HTTPHEADER>( headers_ );
        }
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

    auto clean() -> void {
        //
        curl_slist_free_all( headers_ );
    }

protected:
    Handle handle_;
    curl_slist* headers_{ nullptr };
};

export struct HttpRequestGet final : HttpRequest {
    HttpRequestGet()
        : HttpRequest() {
        // Mark request as GET.
        handle_.setopt<CURLOPT_HTTPGET>( 1l );
    }
};

export struct HttpRequestPost final : HttpRequest {
    HttpRequestPost()
        : HttpRequest() {
        // Mark request as POST.
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
        // Mark request as DELETE.
        handle_.setopt<CURLOPT_CUSTOMREQUEST>( "DELETE" );
    }
};

export struct HttpRequestPut final : HttpRequest {
    HttpRequestPut()
        : HttpRequest() {
        // Mark request as PUT.
        handle_.setopt<CURLOPT_UPLOAD>( 1l );
    }
};

}  // namespace poller
