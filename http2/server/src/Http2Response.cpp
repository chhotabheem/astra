#include "Http2Response.hpp"
#include "ResponseImpl.hpp"
#include <Logger.h>

namespace http2server {

Response::Response() = default;

// Move operations must be defined after Impl is complete (after ResponseImpl.hpp include)
Response::Response(Response&&) noexcept = default;
Response& Response::operator=(Response&&) noexcept = default;

Response::~Response() {
    if (m_impl && !m_impl->closed) {
        // Ensure we close if not already closed, though usually handler does it.
        // If we destroy response without sending, the client hangs?
        // nghttp2-asio might handle destruction.
    }
}

void Response::set_status(int code) noexcept {
    if (!m_impl) return;
    m_impl->status_code = code;
}

void Response::set_header(std::string_view key, std::string_view value) {
    if (!m_impl) return;
    m_impl->headers.emplace(std::string(key), nghttp2::asio_http2::header_value{std::string(value), false});
}

void Response::write(std::string_view data) {
    if (!m_impl) return;
    m_impl->body_buffer.append(data);
}

void Response::close() {
    if (!m_impl) return;
    
    if (!m_impl->headers_sent) {
        m_impl->res->write_head(m_impl->status_code, m_impl->headers);
        m_impl->headers_sent = true;
    }
    
    m_impl->res->end(m_impl->body_buffer);
    m_impl->closed = true;
}

} // namespace http2server
