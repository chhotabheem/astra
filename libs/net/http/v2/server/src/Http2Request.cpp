#include "Http2Request.h"
#include "RequestData.h"

namespace astra::http2 {

Request::Request(std::weak_ptr<RequestData> data)
    : m_data(std::move(data)) {
}

const std::string& Request::method() const {
    if (auto d = m_data.lock()) {
        return d->method;
    }
    return m_empty;
}

const std::string& Request::path() const {
    if (auto d = m_data.lock()) {
        return d->path;
    }
    return m_empty;
}

std::string Request::header(const std::string& key) const {
    if (auto d = m_data.lock()) {
        auto it = d->headers.find(key);
        if (it != d->headers.end()) {
            return it->second;
        }
    }
    return {};
}

const std::string& Request::body() const {
    if (auto d = m_data.lock()) {
        return d->body;
    }
    return m_empty;
}

std::string Request::path_param(const std::string& key) const {
    if (auto d = m_data.lock()) {
        auto it = d->path_params.find(key);
        if (it != d->path_params.end()) {
            return it->second;
        }
    }
    return {};
}

std::string Request::query_param(const std::string& key) const {
    if (auto d = m_data.lock()) {
        auto it = d->query_params.find(key);
        if (it != d->query_params.end()) {
            return it->second;
        }
    }
    return {};
}

void Request::set_path_params(std::unordered_map<std::string, std::string> params) {
    if (auto d = m_data.lock()) {
        d->path_params = std::move(params);
    }
}

} // namespace astra::http2
