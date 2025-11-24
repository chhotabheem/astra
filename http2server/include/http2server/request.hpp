#pragma once

#include <string>
#include <memory>

namespace http2server {

class Server;
namespace backend { class NgHttp2Server; }

class Request {
public:
    Request();
    ~Request();
    
    // Move-only
    Request(Request&&) noexcept;
    Request& operator=(Request&&) noexcept;

    Request(const Request&) = delete;
    Request& operator=(const Request&) = delete;

    const std::string& method() const;
    const std::string& path() const;
    const std::string& header(const std::string& key) const;
    const std::string& body() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
    friend class Server; 
    
    friend class backend::NgHttp2Server;
};

} // namespace http2server
