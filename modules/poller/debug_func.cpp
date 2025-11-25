
module;

#include <string>
#include <curl/curl.h>

export module poller:debug_func;

namespace poller {

void debugDumpHex( const char* text, FILE* stream, unsigned char* ptr,
                   size_t size ) {
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

int debugTraceAll( CURL* handle, curl_infotype type, char* data, size_t size,
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

    debugDumpHex( text, stderr, (unsigned char*)data, size );
    return 0;
}

}  // namespace poller