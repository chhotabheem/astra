#pragma once

#include "http_abstractions/IRequest.hpp"
#include <memory>
#include <string_view>

namespace http2server {

class Server;
namespace backend { class NgHttp2Server; }

class Request final : public http_abstractions::IRequest {
public:
    Request();
    ~Request() override;
    
    // Move-only
    Request(Request&&) noexcept;
    Request& operator=(Request&&) noexcept;

    Request(const Request&) = delete;
    Request& operator=(const Request&) = delete;

    [[nodiscard]] std::string_view method() const override;
    [[nodiscard]] std::string_view path() const override;
    [[nodiscard]] std::string_view header(std::string_view key) const override;
    [[nodiscard]] std::string_view body() const override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
    friend class Server; 
    
    friend class backend::NgHttp2Server;
};

} // namespace http2server
