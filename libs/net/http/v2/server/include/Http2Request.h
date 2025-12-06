#pragma once

#include "IRequest.h"
#include <memory>
#include <unordered_map>
#include <string_view>

namespace http2server {

class Server;
namespace backend { class NgHttp2Server; }

class Request final : public router::IRequest {
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

    [[nodiscard]] std::string_view path_param(std::string_view key) const override;
    [[nodiscard]] std::string_view query_param(std::string_view key) const override;

    void set_path_params(std::unordered_map<std::string_view, std::string_view> params) override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
    friend class Server; 
    
    friend class backend::NgHttp2Server;
};

} // namespace http2server
