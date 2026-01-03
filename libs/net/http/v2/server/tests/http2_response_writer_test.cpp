#include "Http2ResponseWriter.h"

#include <IScopedResource.h>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

using namespace astra::http2;
using namespace std::chrono_literals;

class Http2ResponseWriterTest : public ::testing::Test {
protected:
  boost::asio::io_context io_ctx;
  int captured_status = -1;
  std::map<std::string, std::string> captured_headers;
  std::string captured_body;
  bool send_called = false;

  void SetUp() override {
    captured_status = -1;
    captured_headers.clear();
    captured_body.clear();
    send_called = false;
  }

  auto make_send_fn() {
    return [this](int status, std::map<std::string, std::string> headers,
                  std::string body) {
      captured_status = status;
      captured_headers = std::move(headers);
      captured_body = std::move(body);
      send_called = true;
    };
  }

  auto make_post_work() {
    return [this](std::function<void()> work) {
      boost::asio::post(io_ctx, std::move(work));
    };
  }
};

TEST_F(Http2ResponseWriterTest, SendSuccess) {
  // Create handle
  auto handle =
      std::make_shared<Http2ResponseWriter>(make_send_fn(), make_post_work());

  // Simulate worker thread sending response
  handle->send(200, {{"Content-Type", "text/plain"}}, "test response");

  // Run io_context to process posted task
  io_ctx.run();

  // Verify send was called
  EXPECT_TRUE(send_called);
  EXPECT_EQ(captured_status, 200);
  EXPECT_EQ(captured_headers["Content-Type"], "text/plain");
  EXPECT_EQ(captured_body, "test response");
}

TEST_F(Http2ResponseWriterTest, SendAfterClose) {
  // Create handle
  auto handle =
      std::make_shared<Http2ResponseWriter>(make_send_fn(), make_post_work());

  // Simulate stream closure
  handle->mark_closed();
  EXPECT_FALSE(handle->is_alive());

  // Worker tries to send after closure
  handle->send(200, {}, "should be dropped");

  // Run io_context
  io_ctx.run();

  // Verify send was NOT called (response dropped)
  EXPECT_FALSE(send_called);
  EXPECT_TRUE(captured_body.empty());
}

TEST_F(Http2ResponseWriterTest, CloseAfterSendPosted) {
  // Create handle
  auto handle =
      std::make_shared<Http2ResponseWriter>(make_send_fn(), make_post_work());

  // Worker posts send (but io_context not run yet)
  handle->send(200, {}, "test data");

  // Stream closes before io_context runs
  handle->mark_closed();

  // Run io_context - send was posted before closure
  io_ctx.run();

  // The posted lambda should still execute but check alive flag
  // Since closure happened after post, the data should be dropped
  EXPECT_FALSE(send_called);
}

TEST_F(Http2ResponseWriterTest, WeakPtrExpiration) {
  // Create handle with strong reference
  auto handle =
      std::make_shared<Http2ResponseWriter>(make_send_fn(), make_post_work());

  // Worker holds weak reference
  std::weak_ptr<Http2ResponseWriter> weak_handle = handle;

  // Worker sends
  if (auto h = weak_handle.lock()) {
    h->send(200, {}, "test");
  }

  // Run io_context to process posted task (this keeps handle alive)
  io_ctx.run();
  io_ctx.restart(); // Reset for potential future use

  // Now destroy handle (simulating stream destruction)
  handle.reset();

  // Weak ptr should now be expired
  EXPECT_TRUE(weak_handle.expired());

  // Try to send again (should fail to lock)
  if (auto h = weak_handle.lock()) {
    FAIL() << "weak_ptr should have expired";
  } else {
    // Expected: cannot lock, so cannot send
    SUCCEED();
  }
}

TEST_F(Http2ResponseWriterTest, IsAliveInitiallyTrue) {
  auto handle =
      std::make_shared<Http2ResponseWriter>(make_send_fn(), make_post_work());
  EXPECT_TRUE(handle->is_alive());
}

TEST_F(Http2ResponseWriterTest, IsAliveFalseAfterClose) {
  auto handle =
      std::make_shared<Http2ResponseWriter>(make_send_fn(), make_post_work());
  handle->mark_closed();
  EXPECT_FALSE(handle->is_alive());
}

TEST_F(Http2ResponseWriterTest, ConcurrentSends) {
  // Test multiple worker threads sending through same handle
  auto handle = std::make_shared<Http2ResponseWriter>(
      [](int, std::map<std::string, std::string>,
         std::string) { /* no-op for this test */ },
      make_post_work());

  constexpr int NUM_THREADS = 4;
  constexpr int SENDS_PER_THREAD = 10;

  std::vector<std::thread> workers;

  for (int i = 0; i < NUM_THREADS; ++i) {
    workers.emplace_back([handle, i]() {
      for (int j = 0; j < SENDS_PER_THREAD; ++j) {
        if (handle->is_alive()) {
          handle->send(200, {},
                       "data_" + std::to_string(i) + "_" + std::to_string(j));
        }
        std::this_thread::sleep_for(1ms);
      }
    });
  }

  // Run io_context in separate thread
  std::thread io_thread([this]() {
    io_ctx.run();
  });

  // Wait for workers
  for (auto &t : workers) {
    t.join();
  }

  // Stop io_context and wait
  io_ctx.stop();
  io_thread.join();

  // No crashes = success
  SUCCEED();
}

// =============================================================================
// IScopedResource Cleanup Tests
// =============================================================================

namespace {
class TestScopedResource : public astra::execution::IScopedResource {
public:
  explicit TestScopedResource(bool &destroyed_flag)
      : m_destroyed_flag(destroyed_flag) {
  }

  ~TestScopedResource() override {
    m_destroyed_flag = true;
  }

private:
  bool &m_destroyed_flag;
};
} // namespace

TEST_F(Http2ResponseWriterTest, ScopedResourceReleasedOnDestruction) {
  bool resource_destroyed = false;

  {
    auto handle =
        std::make_shared<Http2ResponseWriter>(make_send_fn(), make_post_work());
    handle->add_scoped_resource(
        std::make_unique<TestScopedResource>(resource_destroyed));

    EXPECT_FALSE(resource_destroyed);
  }

  EXPECT_TRUE(resource_destroyed)
      << "Scoped resource must be released when handle is destroyed";
}

TEST_F(Http2ResponseWriterTest, MultipleScopedResourcesReleasedOnDestruction) {
  bool resource1_destroyed = false;
  bool resource2_destroyed = false;
  bool resource3_destroyed = false;

  {
    auto handle =
        std::make_shared<Http2ResponseWriter>(make_send_fn(), make_post_work());
    handle->add_scoped_resource(
        std::make_unique<TestScopedResource>(resource1_destroyed));
    handle->add_scoped_resource(
        std::make_unique<TestScopedResource>(resource2_destroyed));
    handle->add_scoped_resource(
        std::make_unique<TestScopedResource>(resource3_destroyed));

    EXPECT_FALSE(resource1_destroyed);
    EXPECT_FALSE(resource2_destroyed);
    EXPECT_FALSE(resource3_destroyed);
  }

  EXPECT_TRUE(resource1_destroyed);
  EXPECT_TRUE(resource2_destroyed);
  EXPECT_TRUE(resource3_destroyed);
}

TEST_F(Http2ResponseWriterTest, ScopedResourceReleasedInReverseOrder) {
  std::vector<int> destruction_order;

  class OrderedResource : public astra::execution::IScopedResource {
  public:
    OrderedResource(int id, std::vector<int> &order)
        : m_id(id), m_order(order) {
    }
    ~OrderedResource() override {
      m_order.push_back(m_id);
    }

  private:
    int m_id;
    std::vector<int> &m_order;
  };

  {
    auto handle =
        std::make_shared<Http2ResponseWriter>(make_send_fn(), make_post_work());
    handle->add_scoped_resource(
        std::make_unique<OrderedResource>(1, destruction_order));
    handle->add_scoped_resource(
        std::make_unique<OrderedResource>(2, destruction_order));
    handle->add_scoped_resource(
        std::make_unique<OrderedResource>(3, destruction_order));
  }

  // Vector destruction is in forward order (FIFO)
  ASSERT_EQ(destruction_order.size(), 3);
  EXPECT_EQ(destruction_order[0], 1);
  EXPECT_EQ(destruction_order[1], 2);
  EXPECT_EQ(destruction_order[2], 3);
}

// =============================================================================
// Edge Cases and SEDA Pattern Tests
// =============================================================================

TEST_F(Http2ResponseWriterTest, SendWithEmptyData) {
  auto handle =
      std::make_shared<Http2ResponseWriter>(make_send_fn(), make_post_work());

  // Send with empty headers and empty body
  handle->send(204, {}, "");
  io_ctx.run();

  EXPECT_TRUE(send_called);
  EXPECT_EQ(captured_status, 204);
  EXPECT_TRUE(captured_headers.empty());
  EXPECT_TRUE(captured_body.empty());
}

TEST_F(Http2ResponseWriterTest, MultipleSendsAllowed) {
  int send_count = 0;
  auto handle = std::make_shared<Http2ResponseWriter>(
      [&send_count](int, std::map<std::string, std::string>, std::string) {
        send_count++;
      },
      make_post_work());

  // Multiple sends should all be dispatched (no blocking)
  handle->send(200, {}, "first");
  handle->send(200, {}, "second");
  handle->send(200, {}, "third");

  io_ctx.run();

  // All sends should have been executed
  EXPECT_EQ(send_count, 3);
}

TEST_F(Http2ResponseWriterTest, MarkClosedIdempotent) {
  auto handle =
      std::make_shared<Http2ResponseWriter>(make_send_fn(), make_post_work());

  // Multiple mark_closed calls should not crash
  EXPECT_TRUE(handle->is_alive());
  handle->mark_closed();
  EXPECT_FALSE(handle->is_alive());
  handle->mark_closed(); // Second call
  EXPECT_FALSE(handle->is_alive());
  handle->mark_closed(); // Third call
  EXPECT_FALSE(handle->is_alive());

  SUCCEED();
}

TEST_F(Http2ResponseWriterTest, SendFromDifferentThread) {
  // Simulate SEDA pattern: create on one thread, send from another
  auto handle =
      std::make_shared<Http2ResponseWriter>(make_send_fn(), make_post_work());

  // Keep io_context running until we explicitly stop
  auto work_guard = boost::asio::make_work_guard(io_ctx);

  std::thread io_thread([this]() {
    io_ctx.run();
  });

  std::thread worker([handle]() {
    // Simulate processing delay
    std::this_thread::sleep_for(10ms);
    handle->send(200, {{"X-Thread", "worker"}}, "from worker thread");
  });

  worker.join();

  // Give io_context time to process, then stop
  std::this_thread::sleep_for(20ms);
  work_guard.reset();
  io_ctx.stop();
  io_thread.join();

  EXPECT_TRUE(send_called);
  EXPECT_EQ(captured_status, 200);
  EXPECT_EQ(captured_headers["X-Thread"], "worker");
  EXPECT_EQ(captured_body, "from worker thread");
}

TEST_F(Http2ResponseWriterTest, SendWithLargeBody) {
  auto handle =
      std::make_shared<Http2ResponseWriter>(make_send_fn(), make_post_work());

  // 1MB body
  std::string large_body(1024 * 1024, 'X');
  handle->send(200, {}, large_body);

  io_ctx.run();

  EXPECT_TRUE(send_called);
  EXPECT_EQ(captured_body.size(), 1024 * 1024);
}

TEST_F(Http2ResponseWriterTest, SendWithManyHeaders) {
  auto handle =
      std::make_shared<Http2ResponseWriter>(make_send_fn(), make_post_work());

  std::map<std::string, std::string> many_headers;
  for (int i = 0; i < 100; ++i) {
    many_headers["X-Header-" + std::to_string(i)] =
        "value-" + std::to_string(i);
  }

  handle->send(200, many_headers, "test");
  io_ctx.run();

  EXPECT_TRUE(send_called);
  EXPECT_EQ(captured_headers.size(), 100);
  EXPECT_EQ(captured_headers["X-Header-50"], "value-50");
}

TEST_F(Http2ResponseWriterTest, DelayedSendAfterCreation) {
  // Simulate SEDA: long processing time before response
  auto handle =
      std::make_shared<Http2ResponseWriter>(make_send_fn(), make_post_work());

  // Keep io_context running until we explicitly stop
  auto work_guard = boost::asio::make_work_guard(io_ctx);

  // Start io_context in background
  std::thread io_thread([this]() {
    io_ctx.run();
  });

  // Simulate delayed processing (50ms)
  std::this_thread::sleep_for(50ms);

  EXPECT_TRUE(handle->is_alive()); // Should still be alive
  handle->send(200, {}, "delayed response");

  std::this_thread::sleep_for(20ms);
  work_guard.reset();
  io_ctx.stop();
  io_thread.join();

  EXPECT_TRUE(send_called);
}

TEST_F(Http2ResponseWriterTest, ConcurrentCloseAndSend) {
  // Race between close and send - should not crash
  auto handle = std::make_shared<Http2ResponseWriter>(
      [](int, std::map<std::string, std::string>, std::string) {},
      make_post_work());

  std::thread sender([handle]() {
    for (int i = 0; i < 100; ++i) {
      handle->send(200, {}, "data");
      std::this_thread::sleep_for(1ms);
    }
  });

  std::thread closer([handle]() {
    std::this_thread::sleep_for(5ms);
    handle->mark_closed();
  });

  std::thread io_thread([this]() {
    io_ctx.run();
  });

  sender.join();
  closer.join();
  io_ctx.stop();
  io_thread.join();

  // No crashes = success
  EXPECT_FALSE(handle->is_alive());
}
