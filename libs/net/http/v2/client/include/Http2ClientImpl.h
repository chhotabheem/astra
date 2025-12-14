#pragma once

#include "Http2Client.h"
#include "Http2ClientResponse.h"
#include <nghttp2/asio_http2_client.h>
#include <boost/asio.hpp>
#include <thread>
#include <atomic>
#include <mutex>
#include <Log.h>

namespace astra::http2 {

class ClientResponse::Impl {
public:
    Impl(int status) : status_code(status) {}
    
    int status_code;
    std::string body;
    std::map<std::string, std::string> headers;
};

/// Connection state for lazy connection pattern
enum class ConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    FAILED
};

class ClientImpl {
public:
    ClientImpl(const ClientConfig& config);
    ~ClientImpl();

    void submit(const std::string& method, const std::string& path, 
                const std::string& body, 
                const std::map<std::string, std::string>& headers,
                ResponseHandler handler);

    bool is_connected() const;
    
    /// Get current connection state
    ConnectionState state() const;

private:
    void ensure_connected();
    void connect();
    void start_io_service();
    void stop_io_service();

    ClientConfig m_config;
    boost::asio::io_context m_io_context;
    std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> m_work;
    std::thread m_io_thread;
    
    std::unique_ptr<nghttp2::asio_http2::client::session> m_session;
    std::atomic<ConnectionState> m_state{ConnectionState::DISCONNECTED};
    std::mutex m_connect_mutex;
};

} // namespace astra::http2
