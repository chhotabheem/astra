#include "Http2Request.hpp"
#include "RequestImpl.hpp"

namespace http2server {

Request::Request() = default;

// Move operations must be defined after Impl is complete (after RequestImpl.hpp include)
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

} // namespace http2server
