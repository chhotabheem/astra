#include "Http2Response.h"
#include "ResponseHandle.h"
#include <obs/Log.h>

namespace http2server {

Response::Response(std::weak_ptr<ResponseHandle> handle)
    : m_handle(std::move(handle)) {
}

void Response::set_status(int code) noexcept {
    m_status = code;
}

void Response::set_header(std::string_view key, std::string_view value) {
    m_headers[std::string(key)] = std::string(value);
}

void Response::write(std::string_view data) {
    m_body.append(data);
}

void Response::close() {
    if (m_closed) {
        return;  // Prevent double-send
    }
    m_closed = true;
    
    if (auto handle = m_handle.lock()) {
        // Use 500 if status not explicitly set (catches missing set_status calls)
        int status = m_status.value_or(500);
        if (!m_status.has_value()) {
            obs::warn("Response closed without setting status code - defaulting to 500");
        }
        handle->send(status, std::move(m_headers), std::move(m_body));
    } else {
        obs::debug("Cannot send response: stream already closed (ResponseHandle expired)");
    }
}

} // namespace http2server
