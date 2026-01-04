#include "PoolExecutor.h"

#include <atomic>
#include <gtest/gtest.h>
#include <mutex>
#include <set>
#include <thread>

namespace astra::execution {

using namespace std::chrono_literals;

// =============================================================================
// Test Handler
// =============================================================================

class TestHandler : public IMessageHandler {
public:
  void handle(Message &msg) override {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_processed_count++;
    m_thread_ids.insert(std::this_thread::get_id());

    if (m_delay > 0ms) {
      std::this_thread::sleep_for(m_delay);
    }
  }

  int processed_count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_processed_count;
  }

  std::set<std::thread::id> thread_ids() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_thread_ids;
  }

  void reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_processed_count = 0;
    m_thread_ids.clear();
  }

  void set_delay(std::chrono::milliseconds delay) {
    m_delay = delay;
  }

private:
  mutable std::mutex m_mutex;
  int m_processed_count = 0;
  std::set<std::thread::id> m_thread_ids;
  std::chrono::milliseconds m_delay{0};
};

class PoolExecutorTest : public ::testing::Test {
protected:
  TestHandler handler;
};

// =============================================================================
// Basic Lifecycle Tests
// =============================================================================

TEST_F(PoolExecutorTest, ConstructsWithThreadCount) {
  PoolExecutor executor(4, handler);
  EXPECT_EQ(executor.thread_count(), 0); // Not started yet
}

TEST_F(PoolExecutorTest, StartsThreads) {
  PoolExecutor executor(4, handler);
  executor.start();
  EXPECT_EQ(executor.thread_count(), 4);
  executor.stop();
}

TEST_F(PoolExecutorTest, StartAndStop) {
  PoolExecutor executor(2, handler);
  executor.start();
  executor.stop();
}

TEST_F(PoolExecutorTest, DoubleStartDoesNotCrash) {
  PoolExecutor executor(2, handler);
  executor.start();
  EXPECT_NO_THROW(executor.start());
  executor.stop();
}

TEST_F(PoolExecutorTest, DoubleStopDoesNotCrash) {
  PoolExecutor executor(2, handler);
  executor.start();
  executor.stop();
  EXPECT_NO_THROW(executor.stop());
}

TEST_F(PoolExecutorTest, StopBeforeStartDoesNotCrash) {
  PoolExecutor executor(2, handler);
  EXPECT_NO_THROW(executor.stop());
}

TEST_F(PoolExecutorTest, DestructorStopsAutomatically) {
  {
    PoolExecutor executor(2, handler);
    executor.start();
    Message msg{.affinity_key = 0, .trace_ctx = {}, .payload = {}};
    executor.submit(std::move(msg));
  } // Destructor should call stop
  SUCCEED(); // No crash means success
}

// =============================================================================
// Message Processing Tests
// =============================================================================

TEST_F(PoolExecutorTest, ProcessesSingleMessage) {
  PoolExecutor executor(2, handler);
  executor.start();

  Message msg{.affinity_key = 0, .trace_ctx = {}, .payload = {}};
  executor.submit(std::move(msg));

  std::this_thread::sleep_for(50ms);
  executor.stop();

  EXPECT_EQ(handler.processed_count(), 1);
}

TEST_F(PoolExecutorTest, ProcessesMultipleMessages) {
  PoolExecutor executor(2, handler);
  executor.start();

  for (int i = 0; i < 100; ++i) {
    Message msg{.affinity_key = 0, .trace_ctx = {}, .payload = i};
    executor.submit(std::move(msg));
  }

  std::this_thread::sleep_for(100ms);
  executor.stop();

  EXPECT_EQ(handler.processed_count(), 100);
}

// =============================================================================
// Work Distribution Tests
// =============================================================================

TEST_F(PoolExecutorTest, MultipleThreadsProcess) {
  PoolExecutor executor(4, handler);
  executor.start();

  // Submit many messages
  for (int i = 0; i < 1000; ++i) {
    Message msg{.affinity_key = 0, .trace_ctx = {}, .payload = i};
    executor.submit(std::move(msg));
  }

  std::this_thread::sleep_for(200ms);
  executor.stop();

  EXPECT_EQ(handler.processed_count(), 1000);
  // Multiple threads should have participated
  EXPECT_GT(handler.thread_ids().size(), 1);
}

TEST_F(PoolExecutorTest, WorkStealingUnderContention) {
  handler.set_delay(5ms);
  PoolExecutor executor(4, handler);
  executor.start();

  // Submit messages that take time to process
  for (int i = 0; i < 20; ++i) {
    Message msg{.affinity_key = 0, .trace_ctx = {}, .payload = i};
    executor.submit(std::move(msg));
  }

  std::this_thread::sleep_for(200ms);
  executor.stop();

  EXPECT_EQ(handler.processed_count(), 20);
  // With 4 threads processing 20 5ms items, all threads should participate
  EXPECT_GT(handler.thread_ids().size(), 1);
}

// =============================================================================
// Context & Payload Preservation Tests
// =============================================================================

TEST_F(PoolExecutorTest, PropagatesTraceContext) {
  struct ContextCapture : public IMessageHandler {
    obs::Context received_ctx;
    void handle(Message &msg) override {
      received_ctx = msg.trace_ctx;
    }
  } ctx_handler;

  PoolExecutor executor(2, ctx_handler);
  executor.start();

  obs::Context ctx = obs::Context::create();
  Message msg{.affinity_key = 0, .trace_ctx = ctx, .payload = {}};
  executor.submit(std::move(msg));

  std::this_thread::sleep_for(50ms);
  executor.stop();

  EXPECT_EQ(ctx_handler.received_ctx.trace_id.high, ctx.trace_id.high);
  EXPECT_EQ(ctx_handler.received_ctx.trace_id.low, ctx.trace_id.low);
}

TEST_F(PoolExecutorTest, PayloadPreserved) {
  struct PayloadCapture : public IMessageHandler {
    std::string received;
    void handle(Message &msg) override {
      received = std::any_cast<std::string>(msg.payload);
    }
  } payload_handler;

  PoolExecutor executor(2, payload_handler);
  executor.start();

  Message msg{.affinity_key = 0,
              .trace_ctx = {},
              .payload = std::string("test payload")};
  executor.submit(std::move(msg));

  std::this_thread::sleep_for(50ms);
  executor.stop();

  EXPECT_EQ(payload_handler.received, "test payload");
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(PoolExecutorTest, SingleThreadExecutor) {
  PoolExecutor executor(1, handler);
  executor.start();

  for (int i = 0; i < 10; ++i) {
    Message msg{.affinity_key = 0, .trace_ctx = {}, .payload = {}};
    executor.submit(std::move(msg));
  }

  std::this_thread::sleep_for(50ms);
  executor.stop();

  EXPECT_EQ(handler.processed_count(), 10);
  EXPECT_EQ(handler.thread_ids().size(), 1); // Only one thread
}

TEST_F(PoolExecutorTest, HighThroughput) {
  PoolExecutor executor(8, handler);
  executor.start();

  constexpr int num_messages = 10000;
  for (int i = 0; i < num_messages; ++i) {
    Message msg{.affinity_key = 0, .trace_ctx = {}, .payload = {}};
    executor.submit(std::move(msg));
  }

  std::this_thread::sleep_for(500ms);
  executor.stop();

  EXPECT_EQ(handler.processed_count(), num_messages);
}

TEST_F(PoolExecutorTest, ConcurrentSubmitters) {
  PoolExecutor executor(4, handler);
  executor.start();

  constexpr int submitters = 4;
  constexpr int messages_per_submitter = 250;
  std::vector<std::thread> threads;

  for (int s = 0; s < submitters; ++s) {
    threads.emplace_back([&executor]() {
      for (int i = 0; i < messages_per_submitter; ++i) {
        Message msg{.affinity_key = 0, .trace_ctx = {}, .payload = {}};
        executor.submit(std::move(msg));
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  std::this_thread::sleep_for(200ms);
  executor.stop();

  EXPECT_EQ(handler.processed_count(), submitters * messages_per_submitter);
}

} // namespace astra::execution
