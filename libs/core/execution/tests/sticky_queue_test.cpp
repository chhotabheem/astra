#include "IMessageHandler.h"
#include "Message.h"
#include "StickyQueue.h"

#include <Span.h>
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <mutex>
#include <set>
#include <thread>

using namespace astra::execution;
using namespace std::chrono_literals;

/**
 * TDD Tests for StickyQueue
 *
 * Key behavior: Queue DELIVERS messages to handler with session affinity
 */

// Mock handler that records received messages
class MockHandler : public IMessageHandler {
public:
  void handle(Message &msg) override {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_received_count++;
    m_last_session_id = msg.session_id;
    m_last_trace_ctx = msg.trace_ctx;
    m_thread_ids.insert(std::this_thread::get_id());
  }

  int received_count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_received_count;
  }

  uint64_t last_session_id() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_last_session_id;
  }

  obs::Context last_trace_ctx() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_last_trace_ctx;
  }

  std::set<std::thread::id> thread_ids() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_thread_ids;
  }

  void reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_received_count = 0;
    m_thread_ids.clear();
  }

private:
  mutable std::mutex m_mutex;
  int m_received_count = 0;
  uint64_t m_last_session_id = 0;
  obs::Context m_last_trace_ctx;
  std::set<std::thread::id> m_thread_ids;
};

class StickyQueueTest : public ::testing::Test {
protected:
  MockHandler handler;
};

TEST_F(StickyQueueTest, DeliversMessageToHandler) {
  StickyQueue queue(2, handler);
  queue.start();

  Message msg{42, obs::Context{}, std::any{}};
  queue.submit(std::move(msg));

  // Wait for delivery
  std::this_thread::sleep_for(50ms);
  queue.stop();

  EXPECT_EQ(handler.received_count(), 1);
  EXPECT_EQ(handler.last_session_id(), 42);
}

TEST_F(StickyQueueTest, SessionAffinityRoutesToSameWorker) {
  StickyQueue queue(4, handler);
  queue.start();

  // Submit 10 messages with same session_id
  for (int i = 0; i < 10; ++i) {
    Message msg{123, obs::Context{}, std::any{}};
    queue.submit(std::move(msg));
  }

  std::this_thread::sleep_for(100ms);
  queue.stop();

  EXPECT_EQ(handler.received_count(), 10);
  // All should be processed by same thread (session affinity)
  EXPECT_EQ(handler.thread_ids().size(), 1);
}

TEST_F(StickyQueueTest, DifferentSessionsCanUseDifferentWorkers) {
  StickyQueue queue(4, handler);
  queue.start();

  // Submit messages with different session_ids
  for (int i = 0; i < 100; ++i) {
    Message msg{static_cast<uint64_t>(i), obs::Context{}, std::any{}};
    queue.submit(std::move(msg));
  }

  std::this_thread::sleep_for(100ms);
  queue.stop();

  EXPECT_EQ(handler.received_count(), 100);
  // Multiple threads should have processed messages
  EXPECT_GT(handler.thread_ids().size(), 1);
}

TEST_F(StickyQueueTest, PropagatesTraceContext) {
  StickyQueue queue(2, handler);
  queue.start();

  // Create a trace context
  obs::Context ctx = obs::Context::create();

  Message msg{1, ctx, std::any{}};
  queue.submit(std::move(msg));

  std::this_thread::sleep_for(50ms);
  queue.stop();

  // Handler should have received the trace context
  auto received_ctx = handler.last_trace_ctx();
  EXPECT_EQ(received_ctx.trace_id.high, ctx.trace_id.high);
  EXPECT_EQ(received_ctx.trace_id.low, ctx.trace_id.low);
}

TEST_F(StickyQueueTest, HandlesPayload) {
  // Custom handler that extracts payload
  struct PayloadHandler : public IMessageHandler {
    std::string received_data;
    std::mutex mutex;

    void handle(Message &msg) override {
      std::lock_guard<std::mutex> lock(mutex);
      received_data = std::any_cast<std::string>(msg.payload);
    }
  };

  PayloadHandler payloadHandler;
  StickyQueue queue(2, payloadHandler);
  queue.start();

  Message msg{1, obs::Context{}, std::string("hello world")};
  queue.submit(std::move(msg));

  std::this_thread::sleep_for(50ms);
  queue.stop();

  EXPECT_EQ(payloadHandler.received_data, "hello world");
}

TEST_F(StickyQueueTest, ConcurrentSubmission) {
  StickyQueue queue(4, handler);
  queue.start();

  constexpr int NUM_THREADS = 4;
  constexpr int MSGS_PER_THREAD = 100;

  std::vector<std::thread> submitters;
  for (int t = 0; t < NUM_THREADS; ++t) {
    submitters.emplace_back([&queue, t]() {
      for (int i = 0; i < MSGS_PER_THREAD; ++i) {
        Message msg{static_cast<uint64_t>(t * 1000 + i), obs::Context{},
                    std::any{}};
        queue.submit(std::move(msg));
      }
    });
  }

  for (auto &t : submitters) {
    t.join();
  }

  std::this_thread::sleep_for(200ms);
  queue.stop();

  EXPECT_EQ(handler.received_count(), NUM_THREADS * MSGS_PER_THREAD);
}

TEST_F(StickyQueueTest, GracefulShutdown) {
  StickyQueue queue(2, handler);
  queue.start();

  // Submit messages
  for (int i = 0; i < 50; ++i) {
    Message msg{static_cast<uint64_t>(i), obs::Context{}, std::any{}};
    queue.submit(std::move(msg));
  }

  // Stop immediately (should process pending messages)
  queue.stop();

  // All messages should be processed
  EXPECT_EQ(handler.received_count(), 50);
}

TEST_F(StickyQueueTest, StartStopMultipleTimes) {
  StickyQueue queue(2, handler);

  for (int cycle = 0; cycle < 3; ++cycle) {
    queue.start();

    Message msg{static_cast<uint64_t>(cycle), obs::Context{}, std::any{}};
    queue.submit(std::move(msg));

    std::this_thread::sleep_for(50ms);
    queue.stop();
  }

  EXPECT_EQ(handler.received_count(), 3);
}

TEST_F(StickyQueueTest, WorkerCount) {
  StickyQueue queue(8, handler);
  EXPECT_EQ(queue.worker_count(), 8);
}
