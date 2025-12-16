#include "Http2Response.h"
#include "ResponseHandle.h"
#include <Log.h>

namespace astra::http2 {

ServerResponse::ServerResponse(std::weak_ptr<ResponseHandle> handle)
    : m_handle(std::move(handle)) {
}

void ServerResponse::set_status(int code) noexcept {
    m_status = code;
}

void ServerResponse::set_header(const std::string& key, const std::string& value) {
    m_headers[key] = value;
}

void ServerResponse::write(const std::string& data) {
    m_body.append(data);
}

void ServerResponse::close() {
    if (m_closed) {
        return;
    }
    m_closed = true;
    
    if (auto handle = m_handle.lock()) {
        int status = m_status.value_or(500);
        if (!m_status.has_value()) {
            obs::warn("ServerResponse closed without setting status code - defaulting to 500");
        }
        handle->send(status, std::move(m_headers), std::move(m_body));
    } else {
        obs::debug("Cannot send response: stream already closed");
    }
}

void ServerResponse::add_scoped_resource(std::unique_ptr<astra::execution::IScopedResource> resource) {
    if (auto handle = m_handle.lock()) {
        handle->add_scoped_resource(std::move(resource));
    }
}

bool ServerResponse::is_alive() const noexcept {
    if (auto handle = m_handle.lock()) {
        return handle->is_alive();
    }
    return false;
}

} // namespace astra::http2
