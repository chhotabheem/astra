#pragma once

#include "IRequest.h"
#include <memory>
#include <unordered_map>
#include <string_view>

namespace http2server {

struct RequestData;  // Forward declaration

/**
 * @brief Lightweight copyable handle to HTTP request data.
 * 
 * Holds weak_ptr<RequestData>. Each method locks the weak_ptr
 * and returns empty if the underlying data has expired.
 * This provides graceful behavior when the stream closes.
 */
class Request final : public router::IRequest {
public:
    Request() = default;
    explicit Request(std::weak_ptr<RequestData> data);
    
    // Copyable (default)
    Request(const Request&) = default;
    Request& operator=(const Request&) = default;
    
    // Also movable
    Request(Request&&) noexcept = default;
    Request& operator=(Request&&) noexcept = default;
    
    ~Request() override = default;

    [[nodiscard]] std::string_view method() const override;
    [[nodiscard]] std::string_view path() const override;
    [[nodiscard]] std::string_view header(std::string_view key) const override;
    [[nodiscard]] std::string_view body() const override;

    [[nodiscard]] std::string_view path_param(std::string_view key) const override;
    [[nodiscard]] std::string_view query_param(std::string_view key) const override;

    void set_path_params(std::unordered_map<std::string, std::string> params) override;

private:
    std::weak_ptr<RequestData> m_data;
};

} // namespace http2server
