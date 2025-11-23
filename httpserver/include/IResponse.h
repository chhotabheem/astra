#pragma once

#include <string>

namespace httpserver {

/**
 * @brief Abstract interface for HTTP responses
 * 
 * Decouples application logic from the underlying HTTP server implementation.
 */
class IResponse {
public:
    virtual ~IResponse() = default;

    /**
     * @brief Set the HTTP status code
     */
    virtual void setStatus(int code) = 0;

    /**
     * @brief Set a response header
     */
    virtual void setHeader(const std::string& name, const std::string& value) = 0;

    /**
     * @brief Send the response body
     * 
     * This typically finalizes the response.
     */
    virtual void send(const std::string& body) = 0;

    /**
     * @brief Send a JSON response
     * 
     * Sets Content-Type to application/json and sends the body.
     */
    virtual void json(const std::string& jsonBody) = 0;

    /**
     * @brief Send an error response
     * 
     * Sets status code and sends a simple error message or JSON error object.
     */
    virtual void sendError(int code, const std::string& message) = 0;
};

} // namespace httpserver
