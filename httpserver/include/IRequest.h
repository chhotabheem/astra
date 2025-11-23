#pragma once

#include <string>
#include <map>
#include <optional>
#include <vector>

namespace httpserver {

/**
 * @brief Abstract interface for HTTP requests
 * 
 * Decouples application logic from the underlying HTTP server implementation.
 */
class IRequest {
public:
    virtual ~IRequest() = default;

    /**
     * @brief Get the HTTP method (GET, POST, etc.)
     */
    virtual std::string method() const = 0;

    /**
     * @brief Get the request path (e.g., "/users/123")
     */
    virtual std::string path() const = 0;

    /**
     * @brief Get the raw query string
     */
    virtual std::string query() const = 0;

    /**
     * @brief Get a specific header value
     * @return value if found, std::nullopt otherwise
     */
    virtual std::optional<std::string> header(const std::string& name) const = 0;

    /**
     * @brief Get all headers
     */
    virtual std::map<std::string, std::string> headers() const = 0;

    /**
     * @brief Get the raw request body
     */
    virtual std::string body() const = 0;

    /**
     * @brief Get a path parameter (e.g., "id" from "/users/:id")
     */
    virtual std::optional<std::string> param(const std::string& name) const = 0;
    
    /**
     * @brief Get a query parameter
     */
    virtual std::optional<std::string> queryParam(const std::string& name) const = 0;
};

} // namespace httpserver
