#include "NgHttp2Server.h"

#include "Http2Request.h"
#include "Http2Response.h"
#include "Http2ResponseWriter.h"
#include "Url.h"

#include <Log.h>

namespace {

struct RequestStream {
  std::string method;
  std::string path;
  std::map<std::string, std::string> headers;
  std::string body;
  std::unordered_map<std::string, std::string> query_params;
  std::shared_ptr<astra::http2::Http2ResponseWriter> response_writer;
  astra::http2::Http2Server::Handler handler;
};

} // namespace

namespace astra::http2 {

NgHttp2Server::NgHttp2Server(const ::http2::ServerConfig &config)
    : m_config(config) {
  int threads = m_config.thread_count() > 0 ? m_config.thread_count() : 1;
  m_server.num_threads(threads);
  obs::info("NgHttp2Server initialized with " + std::to_string(threads) +
            " threads");
}

NgHttp2Server::~NgHttp2Server() {
  if (m_is_running.load(std::memory_order_acquire)) {
    m_server.stop();
    m_server.join();
  }
}

void NgHttp2Server::handle(const std::string &method, const std::string &path,
                           Http2Server::Handler handler) {
  m_server.handle(path, [handler = std::move(handler), method](
                            const nghttp2::asio_http2::server::request &req,
                            const nghttp2::asio_http2::server::response &res) {
    if (method != "*" && req.method() != method) {
      res.write_head(405);
      res.end();
      return;
    }

    auto stream = std::make_shared<RequestStream>();
    stream->method = req.method();
    stream->path = req.uri().path;
    if (!req.uri().raw_query.empty()) {
      stream->query_params =
          utils::Url::parse_query_string(req.uri().raw_query);
    }
    for (const auto &h : req.header()) {
      stream->headers[h.first] = h.second.value;
    }
    stream->handler = handler;

    auto &io_ctx = res.io_service();
    stream->response_writer = std::make_shared<Http2ResponseWriter>(
        [&res](int status, std::map<std::string, std::string> headers,
               std::string body) {
          nghttp2::asio_http2::header_map h;
          for (const auto &[k, v] : headers) {
            h.emplace(k, nghttp2::asio_http2::header_value{v, false});
          }

          res.write_head(status, h);
          res.end(std::move(body));
        },

        [&io_ctx](std::function<void()> work) {
          boost::asio::post(io_ctx, std::move(work));
        });

    const_cast<nghttp2::asio_http2::server::response &>(res).on_close(
        [response_writer = stream->response_writer](uint32_t error_code) {
          response_writer->mark_closed();
          if (error_code != 0) {
            obs::debug("Stream closed with error code: " +
                       std::to_string(error_code));
          }
        });

    const_cast<nghttp2::asio_http2::server::request &>(req).on_data(
        [stream](const uint8_t *data, std::size_t len) {
          if (len > 0) {
            stream->body.append(reinterpret_cast<const char *>(data), len);
          } else {
            auto request = std::make_shared<Http2Request>(
                std::move(stream->method), std::move(stream->path),
                std::move(stream->headers), std::move(stream->body),
                std::move(stream->query_params));
            auto response =
                std::make_shared<Http2Response>(stream->response_writer);

            stream->handler(request, response);
          }
        });
  });
}

astra::outcome::Result<void, Http2ServerError> NgHttp2Server::start() {
  if (m_is_running.load(std::memory_order_acquire)) {
    return astra::outcome::Result<void, Http2ServerError>::Err(
        Http2ServerError::AlreadyRunning);
  }

  // Parse uri to extract address and port
  // Format: http://address:port or https://address:port
  std::string uri = m_config.uri();
  std::string address = "0.0.0.0";
  std::string port = "8080";

  // Remove protocol prefix
  size_t protocol_end = uri.find("://");
  if (protocol_end != std::string::npos) {
    uri = uri.substr(protocol_end + 3);
  }

  // Split address and port
  size_t port_sep = uri.find(':');
  if (port_sep != std::string::npos) {
    address = uri.substr(0, port_sep);
    port = uri.substr(port_sep + 1);
  } else {
    address = uri;
  }

  obs::info("Server starting on " + address + ":" + port);

  boost::system::error_code ec;

  if (m_server.listen_and_serve(ec, address, port, true)) {
    obs::error("Server failed to start: " + ec.message());
    return astra::outcome::Result<void, Http2ServerError>::Err(
        Http2ServerError::BindFailed);
  }

  m_is_running.store(true, std::memory_order_release);
  obs::info("Server started successfully");
  return astra::outcome::Result<void, Http2ServerError>::Ok();
}

astra::outcome::Result<void, Http2ServerError> NgHttp2Server::join() {
  if (!m_is_running.load(std::memory_order_acquire)) {
    return astra::outcome::Result<void, Http2ServerError>::Err(
        Http2ServerError::NotStarted);
  }

  m_server.join();
  m_is_running.store(false, std::memory_order_release);
  obs::info("Server stopped cleanly");
  return astra::outcome::Result<void, Http2ServerError>::Ok();
}

astra::outcome::Result<void, Http2ServerError> NgHttp2Server::stop() {
  if (!m_is_running.load(std::memory_order_acquire)) {
    return astra::outcome::Result<void, Http2ServerError>::Err(
        Http2ServerError::NotStarted);
  }

  m_server.stop();
  obs::info("Server stop requested");
  return astra::outcome::Result<void, Http2ServerError>::Ok();
}

} // namespace astra::http2
