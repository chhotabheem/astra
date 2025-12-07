#pragma once

#include <string>
#include <memory>
#include <functional>
#include "Router.h"

namespace http2server {

class Request;
class Response;

class Server {
public:
    /**
     * @brief Construct a new Server object
     * 
     * @param address The address to bind to (e.g., "0.0.0.0" or "127.0.0.1")
     * @param port The port to listen on
     * @param threads Number of worker threads (default: 1)
     */
    explicit Server(const std::string& address, const std::string& port, int threads = 1);
    ~Server();

    // Prevent copying
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    using Handler = std::function<void(Request&, Response&)>;

    /**
     * @brief Register a handler for a specific method and path
     * 
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
