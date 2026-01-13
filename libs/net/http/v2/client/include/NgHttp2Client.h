#pragma once

#include "Http2ClientError.h"
#include "Http2ClientResponse.h"
#include "http2client.pb.h"

#include <Log.h>
#include <Result.h>
#include <atomic>
#include <boost/asio.hpp>
#include <functional>
#include <mutex>
#include <nghttp2/asio_http2_client.h>
#include <queue>
#include <thread>

namespace astra::http2 {

enum class ConnectionState { DISCONNECTED, CONNECTING, CONNECTED, FAILED };

using ResponseHandler = std::function<void(
    astra::outcome::Result<Http2ClientResponse, Http2ClientError>)>;
using OnCloseCallback = std::function<void()>;
using OnErrorCallback = std::function<void(Http2ClientError)>;

struct PendingRequest {
  std::string method;
  std::string path;
  std::string body;
  std::map<std::string, std::string> headers;
  ResponseHandler handler;
};

class NgHttp2Client {
public:
  NgHttp2Client(const std::string &host, uint16_t port,
                const ::http2::ClientConfig &config,
                OnCloseCallback on_close = nullptr,
                OnErrorCallback on_error = nullptr);
  ~NgHttp2Client();

  NgHttp2Client(const NgHttp2Client &) = delete;
  NgHttp2Client &operator=(const NgHttp2Client &) = delete;

  void submit(const std::string &method, const std::string &path,
              const std::string &body,
              const std::map<std::string, std::string> &headers,
              ResponseHandler handler);

  bool is_connected() const;
  ConnectionState state() const;
  bool is_dead() const;

private:
  void ensure_connected();
  void connect();
  void start_io_thread();
  void stop_io_thread();
  void do_submit(const std::string &method, const std::string &path,
                 const std::string &body,
                 const std::map<std::string, std::string> &headers,
                 ResponseHandler handler);
  void flush_pending_requests();

  std::string m_host;
  uint16_t m_port;
  ::http2::ClientConfig m_config;
  OnCloseCallback m_on_close;
  OnErrorCallback m_on_error;

  boost::asio::io_context m_io_context;
  std::unique_ptr<
      boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>
      m_work;
  std::thread m_io_thread;

  std::unique_ptr<nghttp2::asio_http2::client::session> m_session;
  std::atomic<ConnectionState> m_state{ConnectionState::DISCONNECTED};
  std::mutex m_connect_mutex;
  std::queue<PendingRequest> m_pending_requests;
  std::atomic<bool> m_is_dead{false};
};

} // namespace astra::http2
