#pragma once

#include "http_abstractions/IResponse.hpp"
#include <memory>
#include <string_view>

namespace http2server {

class Server;
namespace backend { class NgHttp2Server; }

class Response final : public http_abstractions::IResponse {
public:
    Response();
    ~Response() override;

    // Move-only
    Response(Response&&) noexcept;
    Response& operator=(Response&&) noexcept;

    Response(const Response&) = delete;
    Response& operator=(const Response&) = delete;

    void set_status(int code) noexcept override;
    void set_header(std::string_view key, std::string_view value) override;
    void write(std::string_view data) override;
    void close() override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
    friend class Server;
    
    friend class backend::NgHttp2Server;
};

} // namespace http2server
