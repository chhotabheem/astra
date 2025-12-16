#pragma once

#include "IRequest.h"
#include <memory>
#include <unordered_map>
#include <string>

namespace astra::http2 {

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

    [[nodiscard]] const std::string& method() const override;
    [[nodiscard]] const std::string& path() const override;
    [[nodiscard]] std::string header(const std::string& key) const override;
    [[nodiscard]] const std::string& body() const override;

    [[nodiscard]] std::string path_param(const std::string& key) const override;
    [[nodiscard]] std::string query_param(const std::string& key) const override;

    void set_path_params(std::unordered_map<std::string, std::string> params) override;

private:
    std::weak_ptr<RequestData> m_data;
    mutable std::string m_empty;  // For returning empty reference
};

} // namespace astra::http2
