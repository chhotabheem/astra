#pragma once

#include "Http2Client.h"
#include "http2client.pb.h"
#include <vector>
#include <memory>
#include <atomic>

namespace http2client {

/**
 * @brief Pool of HTTP/2 clients for high-traffic scenarios
 * 
 * Manages multiple Client instances and distributes requests
 * using round-robin selection. Thread-safe for concurrent access.
 */
class Http2ClientPool {
public:
    /**
     * @brief Construct pool from config
     * @param config Config containing pool_size and client settings
     */
    explicit Http2ClientPool(const http2client::Config& config);
    
    ~Http2ClientPool() = default;
    
    // Non-copyable
    Http2ClientPool(const Http2ClientPool&) = delete;
    Http2ClientPool& operator=(const Http2ClientPool&) = delete;
    
    /**
     * @brief Get a client from the pool (round-robin)
     * @return Reference to a Client instance
     */
    Client& get();
    
    /**
     * @brief Get the number of clients in the pool
     */
    size_t size() const noexcept;

private:
    std::vector<std::unique_ptr<Client>> m_clients;
    std::atomic<size_t> m_next{0};
};

} // namespace http2client
