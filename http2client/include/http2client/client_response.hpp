#pragma once

#include <string>
#include <map>
#include <memory>

namespace http2client {

class Response {
public:
    Response();
    ~Response();

    // Copyable (cheap, shares impl)
    Response(const Response& other) = default;
    Response& operator=(const Response& other) = default;

    int status_code() const;
    const std::string& body() const;
    std::string header(const std::string& name) const;
    const std::map<std::string, std::string>& headers() const;

    // Internal implementation details
    class Impl;
    // Helper to create a response with an implementation
    explicit Response(std::shared_ptr<Impl> impl);

private:
    std::shared_ptr<Impl> m_impl;
};

} // namespace http2client
