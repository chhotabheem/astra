#pragma once

#include <string>
#include <nghttp2/asio_http2_server.h>
#include "Http2Response.hpp"

namespace http2server {

class Response::Impl {
public:
    const nghttp2::asio_http2::server::response* res;
    bool closed = false;

    explicit Impl(const nghttp2::asio_http2::server::response& r) : res(&r) {}

    int status_code = 200;
    nghttp2::asio_http2::header_map headers;
    std::string body_buffer;
    bool headers_sent = false;
};

} // namespace http2server
