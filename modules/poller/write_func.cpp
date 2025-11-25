module;

#include <string>
#include <curl/curl.h>

export module poller:write_func;

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

}  // namespace poller
