#pragma once

#include <string>
#include <memory>

namespace http2server {

class Server;
namespace backend { class NgHttp2Server; }

class Response {
public:
    Response();
    ~Response();

    // Move-only
    Response(Response&&) noexcept;
    Response& operator=(Response&&) noexcept;

    Response(const Response&) = delete;
    Response& operator=(const Response&) = delete;

    void set_status(int code);
    void set_header(const std::string& key, const std::string& value);
    void write(const std::string& data);
    void close();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
    friend class Server;
    
    friend class backend::NgHttp2Server;
};

} // namespace http2server
