#pragma once

#include <string>
#include <memory>
#include <functional>
#include "Router.h"
#include "http2server.pb.h"

namespace http2server {

class Request;
class Response;

class Server {
public:
    /**
     * @brief Construct a new Server object
     * @param config Proto-generated server configuration
     */
    explicit Server(const http2server::Config& config);
    ~Server();

    // Prevent copying
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    using Handler = std::function<void(Request&, Response&)>;

    /**
     * @brief Register a handler for a specific method and path
     * @param method HTTP method (GET, POST, etc.)
     * @param path URL path
     * @param handler The callback function
     */
    void handle(const std::string& method, const std::string& path, Handler handler);

    /**
     * @brief Start the server (blocking)
     */
    void run();

    /**
     * @brief Stop the server
     */
    void stop();

    /**
     * @brief Block until the server is ready to accept connections
     */
    void wait_until_ready();

    router::Router& router() { return m_router; }

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
    router::Router m_router;
};

} // namespace http2server
