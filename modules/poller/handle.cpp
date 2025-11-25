
module;

#include <string>
#include <concepts>
#include <curl/curl.h>

export module poller:handle;

namespace poller {

void dump( const char* text, FILE* stream, unsigned char* ptr, size_t size ) {
    size_t i;
    size_t c;
    unsigned int width = 0x10;

    fprintf( stream, "%s, %10.10ld bytes (0x%8.8lx)\n", text, (long)size,
             (long)size );

    for ( i = 0; i < size; i += width ) {
        fprintf( stream, "%4.4lx: ", (long)i );

        /* show hex to the left */
        for ( c = 0; c < width; c++ ) {
            if ( i + c < size )
                fprintf( stream, "%02x ", ptr[i + c] );
            else
                fputs( "   ", stream );
        }

        /* show data on the right */
        for ( c = 0; ( c < width ) && ( i + c < size ); c++ ) {
            char x =
                ( ptr[i + c] >= 0x20 && ptr[i + c] < 0x80 ) ? ptr[i + c] : '.';
            fputc( x, stream );
        }

        fputc( '\n', stream ); /* newline */
    }
}

int my_trace( CURL* handle, curl_infotype type, char* data, size_t size,
              void* clientp ) {
    const char* text;
    (void)handle;
    (void)clientp;

    switch ( type ) {
        case CURLINFO_TEXT:
            fputs( "== Info: ", stderr );
            fwrite( data, size, 1, stderr );
        default: /* in case a new one is introduced to shock us */
            return 0;

        case CURLINFO_HEADER_OUT:
            text = "=> Send header";
            break;
        case CURLINFO_DATA_OUT:
            text = "=> Send data";
            break;
        case CURLINFO_SSL_DATA_OUT:
            text = "=> Send SSL data";
            break;
        case CURLINFO_HEADER_IN:
            text = "<= Recv header";
            break;
        case CURLINFO_DATA_IN:
            text = "<= Recv data";
            break;
        case CURLINFO_SSL_DATA_IN:
            text = "<= Recv SSL data";
            break;
    }

    dump( text, stderr, (unsigned char*)data, size );
    return 0;
}

template <typename F>
concept CurlWriteFunctionType = std::invocable<F, char*, size_t, size_t, void*>;

template <typename T>
concept CurlObjectType = std::is_class_v<std::remove_pointer_t<T>>;

template <CURLoption Opt>
concept CurlOptCallable =
    ( Opt == CURLOPT_WRITEFUNCTION ) || ( Opt == CURLOPT_READFUNCTION );

template <CURLoption Opt>
concept CurlOptObject =
    ( Opt == CURLOPT_WRITEDATA ) || ( Opt == CURLOPT_PRIVATE );

template <CURLoption Opt>
concept CurlOptString =
    ( Opt == CURLOPT_URL ) || ( Opt == CURLOPT_USERAGENT ) ||
    ( Opt == CURLOPT_POSTFIELDS ) || ( Opt == CURLOPT_COPYPOSTFIELDS ) ||
    ( Opt == CURLOPT_USERPWD ) || ( Opt == CURLOPT_HEADERDATA );

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
    ( Opt == CURLOPT_MIME_OPTIONS );

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

        // Setup debug output function.
        curl_easy_setopt( handle_, CURLOPT_DEBUGFUNCTION, my_trace );

        // The DEBUGFUNCTION has no effect until we enable VERBOSE .
        curl_easy_setopt( handle_, CURLOPT_VERBOSE, 1L );
    }

    ~Handle() = default;

    Handle( const Handle& other ) = delete;
    Handle& operator=( const Handle& other ) = delete;

    Handle( Handle&& other ) {
        handle_ = other.handle_;
        other.handle_ = nullptr;
    }

    Handle& operator=( Handle&& other ) {
        if ( this != &other ) {
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    template <CURLoption Opt>
    requires CurlOptCallable<Opt> auto setopt(
        CurlWriteFunctionType auto value ) -> void {
        curl_easy_setopt( handle_, Opt, value );
    };

    template <CURLoption Opt>
    requires CurlOptString<Opt> auto setopt( const std::string& value )
        -> void {
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

    operator CURL*() { return handle_; };

    bool isValid() { return !( handle_ == nullptr ); };

private:
    // CURL itself must be deal with that handle, do nothing in destructor
    CURL* handle_{ nullptr };
};

}  // namespace poller
