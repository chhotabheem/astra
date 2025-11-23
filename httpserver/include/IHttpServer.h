#pragma once

#include "IRouter.h"
#include <string>

namespace httpserver {

/**
 * @brief Abstract interface for the HTTP server
 */
class IHttpServer {
public:
    virtual ~IHttpServer() = default;

    /**
     * @brief Start the server (blocking or non-blocking depending on impl, usually blocking)
     */
    virtual void start(const std::string& host, int port) = 0;

    /**
     * @brief Stop the server
     */
    virtual void stop() = 0;

    /**
     * @brief Get the router to register routes
     */
    virtual IRouter& router() = 0;
};

} // namespace httpserver
