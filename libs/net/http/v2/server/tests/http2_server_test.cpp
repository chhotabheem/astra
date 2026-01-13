#include "Http2Request.h"
#include "Http2Response.h"
#include "Http2Server.h"
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
  config.set_uri("http://127.0.0.1:9001");
  config.set_thread_count(threads);
  return config;
}

::http2::ServerConfig make_config_port(uint32_t port, uint32_t threads = 1) {
  ::http2::ServerConfig config;
  config.set_uri("http://127.0.0.1:" + std::to_string(port));
  config.set_thread_count(threads);
  return config;
}

} // namespace

TEST(Http2ServerTest, Construction) {
  astra::router::Router router;
  auto server =
      std::make_unique<astra::http2::Http2Server>(make_config(), router);
  EXPECT_NE(server, nullptr);
}

TEST(Http2ServerTest, ConstructionWithDifferentPorts) {
  astra::router::Router router1, router2, router3;
  auto server1 = std::make_unique<astra::http2::Http2Server>(
      make_config_port(9101), router1);
  auto server2 = std::make_unique<astra::http2::Http2Server>(
      make_config_port(9102), router2);
  auto server3 = std::make_unique<astra::http2::Http2Server>(
      make_config_port(9103), router3);

  EXPECT_NE(server1, nullptr);
  EXPECT_NE(server2, nullptr);
  EXPECT_NE(server3, nullptr);
}

TEST(Http2ServerTest, BindToAllInterfaces) {
  astra::router::Router router;
  ::http2::ServerConfig config;
  config.set_uri("http://0.0.0.0:9007");
  config.set_thread_count(1);
  auto server = std::make_unique<astra::http2::Http2Server>(config, router);
  EXPECT_NE(server, nullptr);
}

TEST(Http2ServerTest, HandlerRegistration) {
  astra::router::Router router;
  auto server = std::make_unique<astra::http2::Http2Server>(
      make_config_port(9002), router);

  server->handle("GET", "/test",
                 [&](std::shared_ptr<astra::router::IRequest>,
                     std::shared_ptr<astra::router::IResponse> res) {
                   res->close();
                 });

  SUCCEED();
}

TEST(Http2ServerTest, MultipleHandlers) {
  astra::router::Router router;
  auto server = std::make_unique<astra::http2::Http2Server>(
      make_config_port(9003), router);

  server->handle("GET", "/path1", [](auto, auto res) {
    res->close();
  });
  server->handle("POST", "/path2", [](auto, auto res) {
    res->close();
  });
  server->handle("GET", "/path3", [](auto, auto res) {
    res->close();
  });

  SUCCEED();
}

TEST(Http2ServerTest, SamePathDifferentMethods) {
  astra::router::Router router;
  auto server = std::make_unique<astra::http2::Http2Server>(
      make_config_port(9010), router);

  server->handle("GET", "/users", [](auto, auto res) {
    res->close();
  });
  server->handle("POST", "/users", [](auto, auto res) {
    res->close();
  });
  server->handle("PUT", "/users/:id", [](auto, auto res) {
    res->close();
  });
  server->handle("DELETE", "/users/:id", [](auto, auto res) {
    res->close();
  });

  SUCCEED();
}

TEST(Http2ServerTest, HandlerWithPathParams) {
  astra::router::Router router;
  auto server = std::make_unique<astra::http2::Http2Server>(
      make_config_port(9011), router);

  server->handle("GET", "/users/:userId", [](auto, auto res) {
    res->close();
  });
  server->handle("GET", "/users/:userId/posts/:postId", [](auto, auto res) {
    res->close();
  });
  server->handle("GET", "/org/:orgId/team/:teamId/member/:memberId",
                 [](auto, auto res) {
                   res->close();
                 });

  SUCCEED();
}

TEST(Http2ServerTest, ManyHandlersStress) {
  astra::router::Router router;
  auto server = std::make_unique<astra::http2::Http2Server>(
      make_config_port(9012), router);

  for (int i = 0; i < 100; ++i) {
    server->handle("GET", "/path" + std::to_string(i), [](auto, auto res) {
      res->close();
    });
  }

  SUCCEED();
}

TEST(Http2ServerTest, ThreadConfiguration) {
  EXPECT_NO_THROW({
    astra::router::Router r1;
    astra::router::Router r2;
    astra::router::Router r3;
    auto server1 = std::make_unique<astra::http2::Http2Server>(
        make_config_port(9004, 1), r1);
    auto server2 = std::make_unique<astra::http2::Http2Server>(
        make_config_port(9005, 2), r2);
    auto server4 = std::make_unique<astra::http2::Http2Server>(
        make_config_port(9006, 4), r3);
  });
}

TEST(Http2ServerTest, ManyThreadsConfiguration) {
  EXPECT_NO_THROW({
    astra::router::Router router;
    auto server = std::make_unique<astra::http2::Http2Server>(
        make_config_port(9013, 16), router);
  });
}

TEST(Http2ServerTest, StressConstruction) {
  for (int i = 0; i < 100; ++i) {
    astra::router::Router router;
    auto server = std::make_unique<astra::http2::Http2Server>(
        make_config_port(9008), router);
    EXPECT_NE(server, nullptr);
  }
}

TEST(Http2ServerTest, ConcurrentConstruction) {
  std::vector<std::thread> threads;
  std::atomic<int> success_count{0};

  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([i, &success_count]() {
      try {
        astra::router::Router router;
        auto server = std::make_unique<astra::http2::Http2Server>(
            make_config_port(9100 + i), router);
        if (server) {
          success_count++;
        }
      } catch (...) {
        // Ignore - port conflicts expected
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  // At least some should succeed
  EXPECT_GT(success_count.load(), 0);
}

class Http2ServerRuntimeTest : public Test {
protected:
  void SetUp() override {
    router_ = std::make_unique<astra::router::Router>();
    server_ = std::make_unique<astra::http2::Http2Server>(
        make_config_port(9009), *router_);
  }

  void TearDown() override {
    if (server_) {
      server_->stop();
    }
    if (server_thread_.joinable()) {
      server_thread_.join();
    }
  }

  std::unique_ptr<astra::router::Router> router_;
  std::unique_ptr<astra::http2::Http2Server> server_;
  std::thread server_thread_;
};

TEST_F(Http2ServerRuntimeTest, StartStop) {
  auto start_result = server_->start();
  ASSERT_TRUE(start_result.is_ok());

  server_thread_ = std::thread([this] {
    server_->join();
  });

  auto stop_result = server_->stop();
  ASSERT_TRUE(stop_result.is_ok());
  server_thread_.join();

  SUCCEED();
}

TEST_F(Http2ServerRuntimeTest, DoubleStopIsIdempotent) {
  auto start_result = server_->start();
  ASSERT_TRUE(start_result.is_ok());

  server_thread_ = std::thread([this] {
    server_->join();
  });

  auto stop_result1 = server_->stop();
  ASSERT_TRUE(stop_result1.is_ok());

  // Second stop is idempotent - should also return OK
  auto stop_result2 = server_->stop();
  EXPECT_TRUE(stop_result2.is_ok());

  server_thread_.join();
}

TEST_F(Http2ServerRuntimeTest, StopBeforeStartReturnsError) {
  auto result = server_->stop();
  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.error(), astra::http2::Http2ServerError::NotStarted);
}

TEST_F(Http2ServerRuntimeTest, JoinBeforeStartReturnsError) {
  auto result = server_->join();
  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.error(), astra::http2::Http2ServerError::NotStarted);
}

TEST_F(Http2ServerRuntimeTest, DoubleStartReturnsError) {
  auto start_result1 = server_->start();
  ASSERT_TRUE(start_result1.is_ok());

  auto start_result2 = server_->start();
  EXPECT_TRUE(start_result2.is_err());
  EXPECT_EQ(start_result2.error(),
            astra::http2::Http2ServerError::AlreadyRunning);

  server_->stop();
}

TEST_F(Http2ServerRuntimeTest, HandlerRegistrationBeforeStart) {
  server_->handle("GET", "/test", [](auto, auto res) {
    res->set_status(200);
    res->write("OK");
    res->close();
  });

  auto start_result = server_->start();
  ASSERT_TRUE(start_result.is_ok());

  server_thread_ = std::thread([this] {
    server_->join();
  });

  server_->stop();
  server_thread_.join();

  SUCCEED();
}
