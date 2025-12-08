#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <map>
#include <boost/asio/io_context.hpp>

namespace http2server {

/**
 * @brief Thread-safe handle for sending responses asynchronously from worker threads.
 * 
 * ResponseHandle enables worker threads to safely send HTTP/2 responses without
 * directly accessing nghttp2 response objects. It uses std::function to capture
 * the response object by reference and posts the send operation to the io_context
 * thread where the response object lives.
 * 
 * Thread Safety:
 * - Worker threads hold weak_ptr<ResponseHandle>
 * - send() posts to io_context (non-blocking, async)
 * - Atomic flag prevents sending to closed streams
 * - io_context serialization prevents race with stream closure
 * 
 * Lifetime:
 * - Created on request arrival (io_context thread)
 * - Passed to workers as shared_ptr
 * - Workers convert to weak_ptr (no ownership)
 * - Marked closed when stream closes
 * - If worker calls send() after closure, response is dropped gracefully
 */
class ResponseHandle : public std::enable_shared_from_this<ResponseHandle> {
public:
    using SendFunction = std::function<void(int status, 
                                             std::map<std::string, std::string> headers,
                                             std::string body)>;
    
    /**
     * @brief Construct a response handle
     * @param send_fn Function that captures response object and sends data
     * @param io_ctx The io_context where the response object lives
     */
    ResponseHandle(SendFunction send_fn, boost::asio::io_context& io_ctx);
    
    /**
     * @brief Send response data asynchronously
     * 
     * Posts the send operation to io_context. If stream is already closed,
     * the operation is still posted but will be dropped when executed.
     * This is safe because io_context serializes with on_close callback.
     * 
     * @param status HTTP status code
     * @param headers Response headers
     * @param body Response body
     */
    void send(int status, std::map<std::string, std::string> headers, std::string body);
    
    /**
     * @brief Mark stream as closed (called by on_close callback)
     */
    void mark_closed() noexcept;
    
    /**
     * @brief Check if stream is still alive
     * @return true if stream is alive, false if closed
     */
    [[nodiscard]] bool is_alive() const noexcept;
    
private:
    SendFunction m_send_fn;
    boost::asio::io_context& m_io_ctx;
    std::atomic<bool> m_stream_alive{true};
};

} // namespace http2server
