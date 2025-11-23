#pragma once

#include "IHttpServer.h"
#include <memory>

namespace httpserver {

/**
 * @brief Factory to create HTTP server instances
 */
class HttpServerFactory {
public:
    enum class Type {
        PROXYGEN,
        MOCK,
        // Future implementations:
        // DROGON,
        // NGHTTP2
    };

    /**
     * @brief Create a new HTTP server instance
     * 
     * @param type The type of server implementation to create
     * @return Unique pointer to the server instance
     */
    static std::unique_ptr<IHttpServer> create(Type type);
};

} // namespace httpserver
