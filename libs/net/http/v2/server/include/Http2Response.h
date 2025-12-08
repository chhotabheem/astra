#pragma once

#include "IResponse.h"
#include <memory>
#include <string>
#include <string_view>
#include <map>
#include <optional>

namespace http2server {

class ResponseHandle;  // Forward declaration

/**
 * @brief Lightweight copyable response handle.
 * 
 * Holds response data directly (status, headers, body).
 * Uses weak_ptr<ResponseHandle> for sending via io_context.
 * 
 * Design notes:
 * - m_status is optional (forces explicit set_status call)
 * - m_closed prevents double-send
 * - Copyable - copies are independent (own m_closed flag)
 */
class Response final : public router::IResponse {
public:
    Response() = default;
    explicit Response(std::weak_ptr<ResponseHandle> handle);
    
    // Copyable (default)
    Response(const Response&) = default;
    Response& operator=(const Response&) = default;
    
    // Also movable
    Response(Response&&) noexcept = default;
    Response& operator=(Response&&) noexcept = default;
    
    ~Response() override = default;

    void set_status(int code) noexcept override;
    void set_header(std::string_view key, std::string_view value) override;
    void write(std::string_view data) override;
    void close() override;

private:
    std::optional<int> m_status;
    std::map<std::string, std::string> m_headers;
    std::string m_body;
    std::weak_ptr<ResponseHandle> m_handle;
    bool m_closed = false;
};

} // namespace http2server
