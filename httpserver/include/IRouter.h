#pragma once

#include "IRequest.h"
#include "IResponse.h"
#include <functional>
#include <string>
#include <memory>

namespace httpserver {

/**
 * @brief Function signature for HTTP request handlers
 */
using HttpHandler = std::function<void(const IRequest&, IResponse&)>;

/**
 * @brief Function signature for middleware
 * 
 * Receives request, response, and a 'next' function to call the next handler in the chain.
 */
using MiddlewareHandler = std::function<void(const IRequest&, IResponse&, std::function<void()>)>;

/**
 * @brief Abstract interface for the router
 */
class IRouter {
public:
    virtual ~IRouter() = default;

    // HTTP Methods
    virtual void GET(const std::string& path, HttpHandler handler) = 0;
    virtual void POST(const std::string& path, HttpHandler handler) = 0;
    virtual void PUT(const std::string& path, HttpHandler handler) = 0;
    virtual void DELETE(const std::string& path, HttpHandler handler) = 0;
    virtual void PATCH(const std::string& path, HttpHandler handler) = 0;

    /**
     * @brief Add middleware to the pipeline
     */
    virtual void use(MiddlewareHandler middleware) = 0;
};

} // namespace httpserver
