
#pragma once

#include <string>
#include <concepts>
#include <curl/curl.h>

namespace poller {

template <typename F>
concept CurlWriteFunctionType = std::invocable<F, char*, size_t, size_t, void*>;

template <typename T>
concept CurlObjectType = std::is_class_v<std::remove_pointer_t<T>>;

template <CURLoption Opt>
concept CurlOptCallable = ( Opt == CURLOPT_WRITEFUNCTION ) ||
                          ( Opt == CURLOPT_READFUNCTION );

template <CURLoption Opt>
concept CurlOptObject = ( Opt == CURLOPT_WRITEDATA ) ||
                        ( Opt == CURLOPT_PRIVATE );

template <CURLoption Opt>
concept CurlOptString = ( Opt == CURLOPT_URL ) ||
                        ( Opt == CURLOPT_USERAGENT ) ||
                        ( Opt == CURLOPT_POSTFIELDS ) ||
                        ( Opt == CURLOPT_COPYPOSTFIELDS ) ||
                        ( Opt == CURLOPT_USERPWD ) ||
                        ( Opt == CURLOPT_HEADERDATA );

template <CURLoption Opt>
concept CurlOptLong = ( Opt == CURLOPT_HEADER ) ||
                      ( Opt == CURLOPT_POST ) ||  // HTTP POST
                      ( Opt == CURLOPT_NOPROGRESS ) ||
                      ( Opt == CURLOPT_MAXREDIRS ) ||
                      ( Opt == CURLOPT_TCP_KEEPALIVE ) ||
                      ( Opt == CURLOPT_HTTP_VERSION ) ||
                      ( Opt == CURLOPT_HTTPGET ) ||   // HTTP GET
                      ( Opt == CURLOPT_HTTPPOST ) ||  // HTTP POST, deprecated
                      ( Opt == CURLOPT_MIMEPOST ) ||  // HTTP POST, use instead
                      ( Opt == CURLOPT_PUT ) ||       // HTTP PUT, deprecated
                      ( Opt == CURLOPT_UPLOAD ) ||    // HTTP PUT, use instead
                      ( Opt == CURLOPT_MIME_OPTIONS );

template <CURLoption Opt>
concept CurlOptSList = ( Opt == CURLOPT_HTTPHEADER ) ||
                       ( Opt == CURLOPT_POSTQUOTE ) ||
                       ( Opt == CURLOPT_TELNETOPTIONS ) ||
                       ( Opt == CURLOPT_PREQUOTE ) ||
                       ( Opt == CURLOPT_HTTP200ALIASES ) ||
                       ( Opt == CURLOPT_MAIL_RCPT ) ||
                       ( Opt == CURLOPT_RESOLVE ) ||
                       ( Opt == CURLOPT_PROXYHEADER ) ||
                       ( Opt == CURLOPT_CONNECT_TO ) ||
                       ( Opt == CURLOPT_QUOTE );

struct Handle final {
    Handle();
    ~Handle() = default;

    Handle( const Handle& other ) = delete;
    Handle& operator=( const Handle& other ) = delete;

    Handle( Handle&& other );
    Handle& operator=( Handle&& other );

    template <CURLoption Opt>
    requires CurlOptCallable<Opt> void setopt(
        CurlWriteFunctionType auto value ) {
        curl_easy_setopt( handle_, Opt, value );
    };

    template <CURLoption Opt>
    requires CurlOptString<Opt> void setopt( const std::string& value ) {
        curl_easy_setopt( handle_, Opt, value.c_str() );
    };

    template <CURLoption Opt>
    requires CurlOptLong<Opt> void setopt( std::integral auto value ) {
        curl_easy_setopt( handle_, Opt, value );
    };

    template <CURLoption Opt>
    requires CurlOptObject<Opt> void setopt( CurlObjectType auto value ) {
        curl_easy_setopt( handle_, Opt, value );
    };

    template <CURLoption Opt>
    requires CurlOptSList<Opt> void setopt( curl_slist* value ) {
        curl_easy_setopt( handle_, Opt, value );
    };

    operator CURL*() { return handle_; };

    bool isValid() { return !( handle_ == nullptr ); };

private:
    // CURL itself must be deal with that handle, do nothing in destructor
    CURL* handle_{ nullptr };
};

}  // namespace poller
