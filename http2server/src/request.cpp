#include "http2server/request.hpp"
#include "request_impl.hpp"

namespace http2server {

Request::Request() = default;
Request::~Request() = default;

Request::Request(Request&&) noexcept = default;
Request& Request::operator=(Request&&) noexcept = default;

const std::string& Request::method() const {
    return m_impl->method;
}

const std::string& Request::path() const {
    return m_impl->path;
}

const std::string& Request::header(const std::string& key) const {
    auto it = m_impl->headers.find(key);
    if (it != m_impl->headers.end()) {
        return it->second;
    }
    static const std::string empty;
    return empty;
}

const std::string& Request::body() const {
    return m_impl->body;
}

} // namespace http2server
