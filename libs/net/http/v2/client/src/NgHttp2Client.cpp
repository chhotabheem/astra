#include "NgHttp2Client.h"

#include "Http2ClientResponse.h"

#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace astra::http2 {

namespace {

struct ResponseStream {
  bool completed = false;
  int status_code = 0;
  std::string body;
  std::map<std::string, std::string> headers;
};

} // namespace

NgHttp2Client::NgHttp2Client(const std::string &host, uint16_t port,
                             const ::http2::ClientConfig &config,
                             OnCloseCallback on_close, OnErrorCallback on_error)
    : m_host(host), m_port(port), m_config(config),
      m_on_close(std::move(on_close)), m_on_error(std::move(on_error)) {
  start_io_thread();
}

NgHttp2Client::~NgHttp2Client() {
  stop_io_thread();
}

void NgHttp2Client::start_io_thread() {
  m_work = std::make_unique<
      boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
      boost::asio::make_work_guard(m_io_context));
  m_io_thread = std::thread([this]() {
    try {
      m_io_context.run();
    } catch (const std::exception &e) {
      obs::error(std::string("IO Service error: ") + e.what());
    }
  });
}

void NgHttp2Client::stop_io_thread() {
  // Post shutdown to io_context so all m_session access happens on the
  // io_thread. This prevents TSAN race between main thread reading m_session
  // and io_thread writing it.
  boost::asio::post(m_io_context, [this]() {
    if (m_session &&
        m_state.load(std::memory_order_acquire) == ConnectionState::CONNECTED) {
      m_session->shutdown();
    }
  });

  // Stop the io_context and release work guard so io_context.run() can exit
  m_work.reset();
  m_io_context.stop();

  // Wait for the io_thread to finish BEFORE destroying the session.
  // This prevents TSAN race between session destructor and callbacks still
  // running.
  if (m_io_thread.joinable()) {
    m_io_thread.join();
  }

  // Now safe to destroy the session - io_thread has exited, no callbacks
  // running
  m_session.reset();
}

void NgHttp2Client::ensure_connected() {
  if (m_state.load(std::memory_order_acquire) == ConnectionState::CONNECTED) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_connect_mutex);

  ConnectionState current = m_state.load(std::memory_order_acquire);
  if (current == ConnectionState::CONNECTED ||
      current == ConnectionState::CONNECTING) {
    return;
  }

  m_state.store(ConnectionState::CONNECTING, std::memory_order_release);
  connect();
}

void NgHttp2Client::connect() {
  // Post to io_context to ensure session creation and callback registration
  // happen on the same thread that will invoke the callbacks. This prevents
  // TSAN data races between callback registration and invocation.
  boost::asio::post(m_io_context, [this]() {
    try {
      boost::system::error_code ec;
      std::string port_str = std::to_string(m_port);

      m_session = std::make_unique<nghttp2::asio_http2::client::session>(
          m_io_context, m_host, port_str);

      // Create connect timeout timer
      uint32_t timeout_ms = m_config.connect_timeout_ms() > 0
                                ? m_config.connect_timeout_ms()
                                : 200;
      auto connect_timer =
          std::make_shared<boost::asio::deadline_timer>(m_io_context);
      connect_timer->expires_from_now(
          boost::posix_time::milliseconds(timeout_ms));

      auto connect_completed = std::make_shared<std::atomic<bool>>(false);

      connect_timer->async_wait([this, connect_completed](
                                    const boost::system::error_code &ec) {
        if (ec) {
          return; // Timer was cancelled
        }

        // Atomic CAS: only proceed if we're the first to claim completion
        bool expected = false;
        if (!connect_completed->compare_exchange_strong(expected, true)) {
          return; // Already handled by on_connect or on_error
        }

        // Timeout fired before connection completed
        obs::error("Connection timeout to " + m_host + ":" +
                   std::to_string(m_port));
        m_state.store(ConnectionState::FAILED, std::memory_order_release);
        m_is_dead.store(true, std::memory_order_release);

        // Fail all pending requests
        std::lock_guard<std::mutex> lock(m_connect_mutex);
        while (!m_pending_requests.empty()) {
          auto &req = m_pending_requests.front();
          req.handler(
              astra::outcome::Result<Http2ClientResponse, Http2ClientError>::
                  Err(Http2ClientError::ConnectionFailed));
          m_pending_requests.pop();
        }

        if (m_on_error) {
          m_on_error(Http2ClientError::ConnectionFailed);
        }
      });

      m_session->on_connect(
          [this, connect_timer, connect_completed](
              boost::asio::ip::tcp::resolver::results_type::iterator
                  endpoint_it) {
            // Atomic CAS: only proceed if we're the first to claim completion
            bool expected = false;
            if (!connect_completed->compare_exchange_strong(expected, true)) {
              return; // Already handled by timeout or on_error
            }

            connect_timer->cancel();
            m_state.store(ConnectionState::CONNECTED,
                          std::memory_order_release);
            obs::info("Connected to " + m_host + ":" + std::to_string(m_port));
            flush_pending_requests();
          });

      m_session->on_error([this, connect_timer, connect_completed](
                              const boost::system::error_code &ec) {
        // Atomic CAS: only proceed if we're the first to claim completion
        bool expected = false;
        if (!connect_completed->compare_exchange_strong(expected, true)) {
          return; // Already handled by on_connect or timeout
        }

        connect_timer->cancel();

        ConnectionState prev_state = m_state.load(std::memory_order_acquire);
        m_state.store(ConnectionState::FAILED, std::memory_order_release);
        m_is_dead.store(true, std::memory_order_release);
        obs::error("Connection error: " + ec.message());

        std::lock_guard<std::mutex> lock(m_connect_mutex);
        while (!m_pending_requests.empty()) {
          auto &req = m_pending_requests.front();
          req.handler(
              astra::outcome::Result<Http2ClientResponse, Http2ClientError>::
                  Err(Http2ClientError::ConnectionFailed));
          m_pending_requests.pop();
        }

        if (prev_state == ConnectionState::CONNECTING && m_on_error) {
          m_on_error(Http2ClientError::ConnectionFailed);
        } else if (prev_state == ConnectionState::CONNECTED && m_on_close) {
          m_on_close();
        }
      });

    } catch (const std::exception &e) {
      m_state.store(ConnectionState::FAILED, std::memory_order_release);
      m_is_dead.store(true, std::memory_order_release);
      obs::error("Failed to create session: " + std::string(e.what()));
      if (m_on_error) {
        m_on_error(Http2ClientError::ConnectionFailed);
      }
    }
  });
}

bool NgHttp2Client::is_connected() const {
  return m_state.load(std::memory_order_acquire) == ConnectionState::CONNECTED;
}

ConnectionState NgHttp2Client::state() const {
  return m_state.load(std::memory_order_acquire);
}

bool NgHttp2Client::is_dead() const {
  return m_is_dead.load(std::memory_order_acquire);
}

void NgHttp2Client::submit(const std::string &method, const std::string &path,
                           const std::string &body,
                           const std::map<std::string, std::string> &headers,
                           ResponseHandler handler) {
  ConnectionState current = m_state.load(std::memory_order_acquire);

  if (current == ConnectionState::FAILED) {
    obs::debug("submit: returning error - state is FAILED");
    handler(astra::outcome::Result<Http2ClientResponse, Http2ClientError>::Err(
        Http2ClientError::ConnectionFailed));
    return;
  }

  if (current == ConnectionState::CONNECTED) {
    do_submit(method, path, body, headers, handler);
    return;
  }

  {
    std::lock_guard<std::mutex> lock(m_connect_mutex);
    m_pending_requests.push({method, path, body, headers, handler});
  }

  ensure_connected();
}

void NgHttp2Client::flush_pending_requests() {
  std::lock_guard<std::mutex> lock(m_connect_mutex);
  while (!m_pending_requests.empty()) {
    auto req = std::move(m_pending_requests.front());
    m_pending_requests.pop();
    do_submit(req.method, req.path, req.body, req.headers, req.handler);
  }
}

void NgHttp2Client::do_submit(const std::string &method,
                              const std::string &path, const std::string &body,
                              const std::map<std::string, std::string> &headers,
                              ResponseHandler handler) {
  boost::asio::post(m_io_context, [this, method, path, body, headers,
                                   handler]() {
    if (m_state.load(std::memory_order_acquire) != ConnectionState::CONNECTED) {
      obs::debug("do_submit: returning error - not connected");
      handler(
          astra::outcome::Result<Http2ClientResponse, Http2ClientError>::Err(
              Http2ClientError::NotConnected));
      return;
    }

    boost::system::error_code ec;

    nghttp2::asio_http2::header_map ng_headers;
    for (const auto &kv : headers) {
      ng_headers.emplace(kv.first,
                         nghttp2::asio_http2::header_value{kv.second, false});
    }

    std::string uri = "http://" + m_host + ":" + std::to_string(m_port) + path;

    if (!m_session) {
      obs::debug("do_submit: m_session is nullptr!");
      handler(
          astra::outcome::Result<Http2ClientResponse, Http2ClientError>::Err(
              Http2ClientError::NotConnected));
      return;
    }

    auto req = m_session->submit(ec, method, uri, body, ng_headers);

    if (ec) {
      obs::debug("do_submit: submit failed - ec=" + std::to_string(ec.value()) +
                 " msg=" + ec.message());

      if (ec.value() == 2) {
        obs::info("Connection closed by peer - will reconnect on next request");
        m_state.store(ConnectionState::DISCONNECTED, std::memory_order_release);
        m_is_dead.store(true, std::memory_order_release);
        m_session.reset();
        if (m_on_close) {
          m_on_close();
        }
      }

      handler(
          astra::outcome::Result<Http2ClientResponse, Http2ClientError>::Err(
              Http2ClientError::SubmitFailed));
      return;
    }

    if (!req) {
      obs::debug("do_submit: submit returned nullptr but no error code!");
      handler(
          astra::outcome::Result<Http2ClientResponse, Http2ClientError>::Err(
              Http2ClientError::SubmitFailed));
      return;
    }

    uint32_t timeout_ms = m_config.request_timeout_ms() > 0
                              ? m_config.request_timeout_ms()
                              : 10000;
    auto timer = std::make_shared<boost::asio::deadline_timer>(m_io_context);
    timer->expires_from_now(boost::posix_time::milliseconds(timeout_ms));

    auto stream = std::make_shared<ResponseStream>();

    timer->async_wait([req, stream,
                       handler](const boost::system::error_code &ec) {
      if (!ec && !stream->completed) {
        stream->completed = true;
        req->cancel(NGHTTP2_CANCEL);
        handler(
            astra::outcome::Result<Http2ClientResponse, Http2ClientError>::Err(
                Http2ClientError::RequestTimeout));
      }
    });

    req->on_response(
        [stream](const nghttp2::asio_http2::client::response &res) {
          stream->status_code = res.status_code();

          for (const auto &kv : res.header()) {
            stream->headers[kv.first] = kv.second.value;
          }

          res.on_data([stream](const uint8_t *data, std::size_t len) {
            if (len > 0) {
              stream->body.append(reinterpret_cast<const char *>(data), len);
            }
          });
        });

    req->on_close([stream, timer, handler](uint32_t error_code) {
      if (stream->completed) {
        return;
      }

      timer->cancel();
      stream->completed = true;

      if (error_code != 0) {
        obs::debug("on_close: Stream closed with error code " +
                   std::to_string(error_code));
        handler(
            astra::outcome::Result<Http2ClientResponse, Http2ClientError>::Err(
                Http2ClientError::StreamClosed));
      } else {
        handler(
            astra::outcome::Result<Http2ClientResponse, Http2ClientError>::Ok(
                Http2ClientResponse(stream->status_code,
                                    std::move(stream->body),
                                    std::move(stream->headers))));
      }
    });
  });
}

} // namespace astra::http2
