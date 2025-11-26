#pragma once

#include <string>
#include <functional>
#include <memory>
#include <map>
#include <chrono>
#include "Http2ClientResponse.hpp"

namespace http2client {

/**
 * @brief Configuration for the HTTP/2 Client
 */
struct ClientConfig {
    std::string host;
    std::string port;
    int threads = 1;
    
    // Timeouts
    std::chrono::milliseconds connect_timeout{5000};  // 5 seconds default
    std::chrono::milliseconds request_timeout{10000}; // 10 seconds default

    // Retry Policy
    int max_retries = 3; // Number of retries for failed requests (network errors or 5xx)
    std::chrono::milliseconds retry_interval{1000}; // Delay between retries
    bool retry_on_server_error = true; // Retry on 5xx status codes
};

/**
 * @brief Error information passed to callbacks
 */
struct Error {
    int code = 0; // 0 = Success, Non-zero = Error (e.g., timeout, connection refused)
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
     * @param config Client configuration
     */
    explicit Client(const ClientConfig& config);
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
    // Internal implementation details
    std::unique_ptr<ClientImpl> m_impl; // Pimpl idiom
};

} // namespace http2client
