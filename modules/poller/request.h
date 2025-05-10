
#pragma once

#include <string>

#include "handle.h"

namespace poller {

struct HttpRequest {
    HttpRequest( const std::string& url, const std::string& userAgent );

    HttpRequest( const HttpRequest& other ) = delete;
    HttpRequest& operator=( const HttpRequest& other ) = delete;

    HttpRequest( HttpRequest&& other );
    HttpRequest& operator=( HttpRequest&& other );

    bool isValid() { return handle_.isValid(); }

    Handle& handle() { return handle_; };

    operator CURL*() { return handle_; };

private:
    Handle handle_;
};

struct HttpRequestGet final : HttpRequest {
    HttpRequestGet( const std::string& url, const std::string& userAgent )
        : HttpRequest( url, userAgent ) {}
};

struct HttpRequestPost final : HttpRequest {};

struct HttpRequestDelete final : HttpRequest {};

struct HttpRequestPatch final : HttpRequest {};

struct HttpRequestPut final : HttpRequest {};

}  // namespace poller
