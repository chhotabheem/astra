#include "Http2Response.h"
#include "ResponseImpl.h"
#include "ResponseHandle.h"
#include <Logger.h>

namespace http2server {

Response::Response() = default;

// Move operations must be defined after Impl is complete (after ResponseImpl.h include)
Response::Response(Response&&) noexcept = default;
Response& Response::operator=(Response&&) noexcept = default;

Response::~Response() {
    if (m_impl && !m_impl->closed) {
        // Ensure we close if not already closed
        // Response data is buffered, will be sent on explicit close()
    }
}

void Response::set_status(int code) noexcept {
    if (!m_impl) return;
    m_impl->status_code = code;
}

void Response::set_header(std::string_view key, std::string_view value) {
    if (!m_impl) return;
    // Headers are sent by nghttp2 server automatically with status
    // For now, we only support default headers
    logger::Logger::debug("set_header called: " + std::string(key) + ": " + std::string(value));
}

void Response::write(std::string_view data) {
    if (!m_impl) return;
    m_impl->body_buffer.append(data);
}

void Response::close() {
    if (!m_impl) return;
    if (m_impl->closed) return;
    
    // Try to get ResponseHandle
    if (auto handle = m_impl->response_handle.lock()) {
        // Send buffered data asynchronously
        handle->send(std::move(m_impl->body_buffer));
        m_impl->closed = true;
    } else {
        // Stream already closed or handle expired
        logger::Logger::debug("Cannot send response: stream already closed");
    }
}

} // namespace http2server
