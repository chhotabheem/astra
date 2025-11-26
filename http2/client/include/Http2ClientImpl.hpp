#pragma once

#include "Http2Client.hpp"
#include "Http2ClientResponse.hpp"
#include <nghttp2/asio_http2_client.h>
#include <boost/asio.hpp>
#include <thread>
#include <atomic>
#include <mutex>
#include <Logger.h>

namespace http2client {

class Response::Impl {
public:
    Impl(int status) : status_code(status) {}
    
    int status_code;
    std::string body;
    std::map<std::string, std::string> headers;
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

private:
    void connect();
    void start_io_service();
    void stop_io_service();

    // Helper to handle retries
    void schedule_retry(const std::string& method, const std::string& path, 
                        const std::string& body, 
                        const std::map<std::string, std::string>& headers,
                        ResponseHandler handler,
                        int retries_remaining,
                        const Error& last_error);

    ClientConfig m_config;
    boost::asio::io_context m_io_context;
    std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> m_work;
    std::thread m_io_thread;
    
    std::unique_ptr<nghttp2::asio_http2::client::session> m_session;
    std::atomic<bool> m_connected{false};
    std::mutex m_session_mutex;
};

} // namespace http2client
