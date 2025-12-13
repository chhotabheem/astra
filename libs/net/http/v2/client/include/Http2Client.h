#pragma once

#include <string>
#include <functional>
#include <memory>
#include <map>
#include "Http2ClientResponse.h"
#include "http2client.pb.h"

namespace http2client {

/**
 * @brief Error information passed to callbacks
 */
struct Error {
    int code = 0;
    std::string message;
    
    operator bool() const { return code != 0; }
};

// Callback type: Invoked only when the FULL response is received
using ResponseHandler = std::function<void(const Response&, const Error&)>;

class ClientImpl;

class Client {
public:
    /**
     * @brief Construct a new Client object
     * @param config Proto-generated client configuration
     */
    explicit Client(const http2client::Config& config);
    ~Client();

    // Prevent copying to ensure resource safety
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    /**
     * @brief Asynchronously send a GET request
     * @param path URL path (e.g., "/api/v1/users")
     * @param handler Callback invoked with full response
     */
    void get(const std::string& path, ResponseHandler handler);

    /**
     * @brief Asynchronously send a POST request
     * @param path URL path
     * @param body Request body
     * @param handler Callback invoked with full response
     */
    void post(const std::string& path, const std::string& body, ResponseHandler handler);

    /**
     * @brief Generic request method
     * @param method HTTP method
     * @param path URL path
     * @param body Request body
     * @param headers Custom headers
     * @param handler Callback invoked with full response
     */
    void submit(const std::string& method, const std::string& path, 
                const std::string& body, 
                const std::map<std::string, std::string>& headers,
                ResponseHandler handler);

    /**
     * @brief Check if the client is currently connected
     */
    bool is_connected() const;

private:
    std::unique_ptr<ClientImpl> m_impl;
};

} // namespace http2client
