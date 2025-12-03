module;

#include <string>

#include <curl/curl.h>

export module poller:write_func;

import :payload;

namespace poller {

[[maybe_unused]] auto writeToBuffer( char* data, size_t size, size_t nmemb,
                                     std::string* buffer ) -> size_t {
    size_t result{};
    if ( buffer != nullptr ) {
        buffer->append( data, size * nmemb );
        result = size * nmemb;
    }
    return result;
}

auto writeDataCallback( char* ptr, size_t, size_t nmemb, void* tab ) -> size_t {
    auto r = reinterpret_cast<Payload*>( tab );
    r->data.append( ptr, nmemb );
    return nmemb;
}

auto writeHeaderCallback( char* buffer, size_t size, size_t nitems,
                          void* userdata ) -> size_t {
    auto i = static_cast<Payload*>( userdata );
    i->headers.append( buffer, nitems * size );
    return nitems * size;
}

}  // namespace poller
