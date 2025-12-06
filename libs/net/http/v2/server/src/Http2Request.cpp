#include "Http2Request.h"
#include "RequestImpl.h"

namespace http2server {

Request::Request() = default;

// Move operations must be defined after Impl is complete (after RequestImpl.h include)
Request::Request(Request&&) noexcept = default;
Request& Request::operator=(Request&&) noexcept = default;

Request::~Request() = default;

std::string_view Request::method() const {
    if (!m_impl) return {};
    return m_impl->method;
}

std::string_view Request::path() const {
    if (!m_impl) return {};
    return m_impl->path;
}

std::string_view Request::header(std::string_view key) const {
    if (!m_impl) return {};
    auto it = m_impl->headers.find(std::string(key));
    if (it != m_impl->headers.end()) {
        return it->second;
    }
    static const std::string empty;
    return empty;
}

std::string_view Request::body() const {
    if (!m_impl) return {};
    return m_impl->body;
}

std::string_view Request::path_param(std::string_view key) const {
    if (!m_impl) return {};
    auto it = m_impl->path_params.find(key);
    if (it != m_impl->path_params.end()) {
        return it->second;
    }
    return {};
}

std::string_view Request::query_param(std::string_view key) const {
    // TODO: Implement query param parsing
    (void)key;
    return {};
}

void Request::set_path_params(std::unordered_map<std::string_view, std::string_view> params) {
    if (m_impl) {
        m_impl->path_params = std::move(params);
    }
}

} // namespace http2server
