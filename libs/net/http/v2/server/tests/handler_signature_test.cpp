#include "Http2Request.h"
#include "Http2Response.h"
#include "Http2Server.h"
#include "IRequest.h"
#include "IResponse.h"

#include <atomic>
#include <chrono>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>

using namespace testing;
using namespace std::chrono_literals;

namespace {

// Helper to create proto config for tests
astra::http2::ServerConfig make_config(const std::string &address,
                                       uint32_t port, uint32_t threads = 1) {
  astra::http2::ServerConfig config;
  config.set_address(address);
  config.set_port(port);
  config.set_thread_count(threads);
  return config;
}

} // namespace

/**
 * Handler signature tests for HTTP/2 Server
 * Verifies that handlers work with shared_ptr<IRequest/IResponse>
 */

// Test 1: Handler receives shared_ptr to types
TEST(HandlerSignatureTest, HandlerReceivesReferences) {
  auto server = std::make_unique<astra::http2::Http2Server>(
      make_config("127.0.0.1", 9100));

  bool handler_called = false;

  // Handler signature: shared_ptr<IRequest>, shared_ptr<IResponse>
  server->handle("GET", "/test",
                 [&](std::shared_ptr<astra::router::IRequest> req,
                     std::shared_ptr<astra::router::IResponse> res) {
                   handler_called = true;
                   res->close();
                 });

  // If compilation succeeds, test passes
  EXPECT_TRUE(true);
}

// Test 2: Multiple handlers can be registered
TEST(HandlerSignatureTest, MultipleHandlers) {
  auto server = std::make_unique<astra::http2::Http2Server>(
      make_config("127.0.0.1", 9101));

  server->handle("GET", "/path1",
                 [](std::shared_ptr<astra::router::IRequest> req,
                    std::shared_ptr<astra::router::IResponse> res) {
                   res->close();
                 });

  server->handle("POST", "/path2",
                 [](std::shared_ptr<astra::router::IRequest> req,
                    std::shared_ptr<astra::router::IResponse> res) {
                   res->close();
                 });

  SUCCEED();
}

// Test 3: Handler can access request and response methods
TEST(HandlerSignatureTest, AccessRequestResponseMethods) {
  auto server = std::make_unique<astra::http2::Http2Server>(
      make_config("127.0.0.1", 9102));

  server->handle("GET", "/test",
                 [](std::shared_ptr<astra::router::IRequest> req,
                    std::shared_ptr<astra::router::IResponse> res) {
                   // Access request methods
                   [[maybe_unused]] auto path = req->path();
                   [[maybe_unused]] auto method = req->method();

                   // Access response methods
                   res->set_status(200);
                   res->write("OK");
                   res->close();
                 });

  SUCCEED();
}

// Test 4: Router integration
TEST(HandlerSignatureTest, RouterIntegration) {
  auto server = std::make_unique<astra::http2::Http2Server>(
      make_config("127.0.0.1", 9103));

  // Access router directly - now uses shared_ptr
  server->router().get("/route",
                       [](std::shared_ptr<astra::router::IRequest> req,
                          std::shared_ptr<astra::router::IResponse> res) {
                         res->set_status(200);
                         res->close();
                       });

  SUCCEED();
}

// Test 5: Handler signature matches Server::Handler type
TEST(HandlerSignatureTest, HandlerTypeCompatible) {
  astra::http2::Http2Server::Handler handler =
      [](std::shared_ptr<astra::router::IRequest> req,
         std::shared_ptr<astra::router::IResponse> res) {
        res->close();
      };

  EXPECT_TRUE(handler != nullptr);
}
