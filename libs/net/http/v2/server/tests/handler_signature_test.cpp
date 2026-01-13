#include "Http2Request.h"
#include "Http2Response.h"
#include "Http2Server.h"
#include "IRequest.h"
#include "IResponse.h"
#include "Router.h"

#include <atomic>
#include <chrono>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>

using namespace testing;
using namespace std::chrono_literals;

namespace {

::http2::ServerConfig make_config(uint32_t threads = 1) {
  ::http2::ServerConfig config;
  config.set_uri("http://127.0.0.1:9100");
  config.set_thread_count(threads);
  return config;
}

} // namespace

/**
 * Handler signature tests for HTTP/2 Server
 * Verifies that handlers work with shared_ptr<IRequest/IResponse>
 */

TEST(HandlerSignatureTest, HandlerReceivesReferences) {
  astra::router::Router router;
  auto server =
      std::make_unique<astra::http2::Http2Server>(make_config(), router);

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

TEST(HandlerSignatureTest, MultipleHandlers) {
  astra::router::Router router;
  auto server =
      std::make_unique<astra::http2::Http2Server>(make_config(), router);

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

TEST(HandlerSignatureTest, AccessRequestResponseMethods) {
  astra::router::Router router;
  auto server =
      std::make_unique<astra::http2::Http2Server>(make_config(), router);

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

TEST(HandlerSignatureTest, RouterIntegration) {
  astra::router::Router router;
  auto server =
      std::make_unique<astra::http2::Http2Server>(make_config(), router);

  // Access router directly with new API
  router.add(astra::router::HttpMethod::GET, "/route",
             [](std::shared_ptr<astra::router::IRequest> req,
                std::shared_ptr<astra::router::IResponse> res) {
               res->set_status(200);
               res->close();
             });

  SUCCEED();
}

TEST(HandlerSignatureTest, HandlerTypeCompatible) {
  astra::router::Handler handler =
      [](std::shared_ptr<astra::router::IRequest> req,
         std::shared_ptr<astra::router::IResponse> res) {
        res->close();
      };

  EXPECT_TRUE(handler != nullptr);
}
