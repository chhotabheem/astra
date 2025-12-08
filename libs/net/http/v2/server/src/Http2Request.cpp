#include "Http2Request.h"
#include "RequestData.h"

namespace http2server {

Request::Request(std::weak_ptr<RequestData> data)
    : m_data(std::move(data)) {
}

std::string_view Request::method() const {
    if (auto d = m_data.lock()) {
        return d->method;
    }
    return {};
}

std::string_view Request::path() const {
    if (auto d = m_data.lock()) {
        return d->path;
    }
    return {};
}

std::string_view Request::header(std::string_view key) const {
    if (auto d = m_data.lock()) {
        auto it = d->headers.find(std::string(key));
        if (it != d->headers.end()) {
            return it->second;
        }
    }
    return {};
}

std::string_view Request::body() const {
    if (auto d = m_data.lock()) {
        return d->body;
    }
    return {};
}

std::string_view Request::path_param(std::string_view key) const {
    if (auto d = m_data.lock()) {
        auto it = d->path_params.find(std::string(key));
        if (it != d->path_params.end()) {
            return it->second;
        }
    }
    return {};
}

std::string_view Request::query_param(std::string_view /*key*/) const {
    // TODO: Implement query param parsing
    return {};
}

void Request::set_path_params(std::unordered_map<std::string, std::string> params) {
    if (auto d = m_data.lock()) {
        d->path_params = std::move(params);
    }
}

} // namespace http2server
