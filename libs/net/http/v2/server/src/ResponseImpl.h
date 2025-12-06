#pragma once

#include <string>
#include <memory>
#include "Http2Response.h"

namespace http2server {

class ResponseHandle;

class Response::Impl {
public:
    std::weak_ptr<ResponseHandle> response_handle;
    bool closed = false;

    Impl() = default;

    int status_code = 200;
    std::string body_buffer;
};

} // namespace http2server
