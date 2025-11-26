#include <iostream>
#include <signal.h>
#include "Http2Server.hpp"
#include "Http2Request.hpp"
#include "Http2Response.hpp"ttp2_server.h"

using namespace nghttp2::asio_http2;
using namespace nghttp2::asio_http2::server;

namespace {
http2 *server_ptr = nullptr;

void signal_handler(int signum) {
  if (server_ptr) {
    std::cout << "\nShutting down server..." << std::endl;
    server_ptr->stop();
  }
}
} // namespace

int main(int argc, char *argv[]) {
  try {
    // Parse command line arguments
    std::string address = "127.0.0.1";
    std::string port = "8080";
    
    if (argc >= 2) {
      port = argv[1];
    }
    if (argc >= 3) {
      address = argv[2];
    }

    boost::system::error_code ec;

    // Create HTTP/2 server
    http2 server;
    server_ptr = &server;

    // Setup signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Configure request handlers
    server.handle("/", [](const request &req, const response &res) {
      std::cout << "Received request: " << req.method() << " " << req.uri().path << std::endl;
      
      res.write_head(200, {
        {"content-type", {"application/json", false}}
      });
      
      res.end(R"({
  "status": "ok",
  "message": "Hello from nghttp2-asio HTTP/2 server!",
  "path": "/"
})");
    });

    server.handle("/health", [](const request &req, const response &res) {
      std::cout << "Health check: " << req.method() << " " << req.uri().path << std::endl;
      
      res.write_head(200, {
        {"content-type", {"application/json", false}}
      });
      
      res.end(R"({
  "status": "healthy",
  "service": "nghttp2-asio-test"
})");
    });

    // Start server
    std::cout << "Starting HTTP/2 server on " << address << ":" << port << std::endl;
    std::cout << "Test with: curl -v --http2-prior-knowledge http://" << address << ":" << port << "/" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;

    if (server.listen_and_serve(ec, address, port)) {
      std::cerr << "Error: " << ec.message() << std::endl;
      return 1;
    }

  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
