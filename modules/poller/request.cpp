
#include <numeric>
#include <span>

#include "request.h"

namespace poller {

std::string urlEncode( CURL* curl, std::string_view url ) {
    char* result =
        curl_easy_escape( curl, url.data(), static_cast<int>( url.size() ) );
    const std::string encoded{ result };
    curl_free( result );
    return encoded;
}

std::string formattedFields(
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

HttpRequest::HttpRequest( const std::string& url,
                          const std::string& userAgent ) {
    handle_.setopt<CURLOPT_URL>( url );
    handle_.setopt<CURLOPT_USERAGENT>( userAgent );
};

HttpRequest::HttpRequest( HttpRequest&& other ) {
    handle_ = std::move( other.handle_ );
}

HttpRequest& HttpRequest::operator=( HttpRequest&& other ) {
    if ( this != &other ) {
        handle_ = std::move( other.handle_ );
    }
    return *this;
}

}  // namespace poller
