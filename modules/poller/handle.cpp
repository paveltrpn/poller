
#include "handle.h"

namespace poller {

Handle::Handle() {
    handle_ = curl_easy_init();
}

Handle::Handle( Handle&& other ) {
    handle_ = other.handle_;
    other.handle_ = nullptr;
}

Handle& Handle::operator=( Handle&& other ) {
    if ( this != &other ) {
        handle_ = other.handle_;
        other.handle_ = nullptr;
    }
    return *this;
}

}  // namespace poller
