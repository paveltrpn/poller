
module;

#include <string>
#include <concepts>
#include <curl/curl.h>
#include <curl/easy.h>

export module poller:handle;

import :debug_func;

namespace poller {

template <typename F>
concept CurlWriteFunctionType = std::invocable<F, char*, size_t, size_t, void*>;

template <typename T>
concept CurlObjectType = std::is_class_v<std::remove_pointer_t<T>>;

template <CURLoption Opt>
concept CurlOptCallable =
    ( Opt == CURLOPT_WRITEFUNCTION ) || ( Opt == CURLOPT_READFUNCTION ) ||
    ( Opt == CURLOPT_HEADERFUNCTION );

template <CURLoption Opt>
concept CurlOptObject =
    ( Opt == CURLOPT_WRITEDATA ) || ( Opt == CURLOPT_PRIVATE ) ||
    ( Opt == CURLOPT_HEADERDATA );

template <CURLoption Opt>
concept CurlOptString =
    ( Opt == CURLOPT_URL ) || ( Opt == CURLOPT_USERAGENT ) ||
    ( Opt == CURLOPT_POSTFIELDS ) || ( Opt == CURLOPT_COPYPOSTFIELDS ) ||
    ( Opt == CURLOPT_USERPWD ) || ( Opt == CURLOPT_CUSTOMREQUEST );

template <CURLoption Opt>
concept CurlOptLong =
    ( Opt == CURLOPT_HEADER ) || ( Opt == CURLOPT_POST ) ||  // HTTP POST
    ( Opt == CURLOPT_NOPROGRESS ) || ( Opt == CURLOPT_MAXREDIRS ) ||
    ( Opt == CURLOPT_TCP_KEEPALIVE ) || ( Opt == CURLOPT_HTTP_VERSION ) ||
    ( Opt == CURLOPT_HTTPGET ) ||  // HTTP GET
    //( Opt == CURLOPT_HTTPPOST ) ||  // HTTP POST, deprecated
    ( Opt == CURLOPT_MIMEPOST ) ||  // HTTP POST, use instead
    // ( Opt == CURLOPT_PUT ) ||       // HTTP PUT, deprecated
    ( Opt == CURLOPT_UPLOAD ) ||  // HTTP PUT, use instead
    ( Opt == CURLOPT_MIME_OPTIONS ) || ( Opt == CURLOPT_POSTFIELDSIZE ) ||
    ( Opt == CURLOPT_TIMEOUT );

template <CURLoption Opt>
concept CurlOptSList =
    ( Opt == CURLOPT_HTTPHEADER ) || ( Opt == CURLOPT_POSTQUOTE ) ||
    ( Opt == CURLOPT_TELNETOPTIONS ) || ( Opt == CURLOPT_PREQUOTE ) ||
    ( Opt == CURLOPT_HTTP200ALIASES ) || ( Opt == CURLOPT_MAIL_RCPT ) ||
    ( Opt == CURLOPT_RESOLVE ) || ( Opt == CURLOPT_PROXYHEADER ) ||
    ( Opt == CURLOPT_CONNECT_TO ) || ( Opt == CURLOPT_QUOTE );

struct Handle final {
    Handle() {
        //
        handle_ = curl_easy_init();
    }

    ~Handle() = default;

    Handle( const Handle& other ) = delete;
    auto operator=( const Handle& other ) -> Handle& = delete;

    Handle( Handle&& other ) noexcept {
        handle_ = other.handle_;
        other.handle_ = nullptr;
    }

    auto operator=( Handle&& other ) noexcept -> Handle& {
        if ( this != &other ) {
            this->free();
            this->handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    auto enableDebug() {
        // Setup debug output function.
        curl_easy_setopt( handle_, CURLOPT_DEBUGFUNCTION, debugTraceAll );

        // The DEBUGFUNCTION has no effect until we enable VERBOSE.
        curl_easy_setopt( handle_, CURLOPT_VERBOSE, 1L );
    }

    template <CURLoption Opt>
    requires CurlOptCallable<Opt> auto setopt(
        CurlWriteFunctionType auto value ) -> void {
        curl_easy_setopt( handle_, Opt, value );
    };

    template <CURLoption Opt>
    requires CurlOptString<Opt> auto setopt( const std::string& value )
        -> void {
        // Strings passed to libcurl as 'char *' arguments, are copied by the library;
        // the string storage associated to the pointer argument may be discarded or
        // reused after curl_easy_setopt returns.
        curl_easy_setopt( handle_, Opt, value.c_str() );
    };

    template <CURLoption Opt>
    requires CurlOptLong<Opt> auto setopt( std::integral auto value ) -> void {
        curl_easy_setopt( handle_, Opt, value );
    };

    template <CURLoption Opt>
    requires CurlOptObject<Opt> auto setopt( CurlObjectType auto value )
        -> void {
        curl_easy_setopt( handle_, Opt, value );
    };

    template <CURLoption Opt>
    requires CurlOptSList<Opt> auto setopt( curl_slist* value ) -> void {
        curl_easy_setopt( handle_, Opt, value );
    };

    operator CURL*() {
        //
        return handle_;
    };

    auto isValid() -> bool {
        //
        return handle_ != nullptr;
    };

    auto clone() -> CURL* {
        // This function returns a new curl handle, a duplicate, using
        // all the options previously set in the input curl handle.
        // Both handles can subsequently be used independently and they
        // must both be freed with curl_easy_cleanup.
        return curl_easy_duphandle( handle_ );
    }

    auto free() -> void {
        curl_easy_cleanup( handle_ );
        handle_ = nullptr;
    }

private:
    // CURL itself must be deal with that handle, do
    // nothing in destructor.
    CURL* handle_{ nullptr };
};

}  // namespace poller
