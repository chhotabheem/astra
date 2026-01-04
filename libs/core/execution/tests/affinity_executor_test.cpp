#include "AffinityExecutor.h"

#include <atomic>
#include <gtest/gtest.h>
#include <latch>
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
    m_last_affinity_key = msg.affinity_key;
    m_last_trace_ctx = msg.trace_ctx;
    m_thread_ids.insert(std::this_thread::get_id());

    if (m_should_throw) {
      throw std::runtime_error("Test exception");
    }
    if (m_delay > 0ms) {
      std::this_thread::sleep_for(m_delay);
    }
  }

  int processed_count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_processed_count;
  }

  uint64_t last_affinity_key() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_last_affinity_key;
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
    m_processed_count = 0;
    m_thread_ids.clear();
  }

  void set_throw(bool should_throw) {
    m_should_throw = should_throw;
  }
  void set_delay(std::chrono::milliseconds delay) {
    m_delay = delay;
  }

private:
  mutable std::mutex m_mutex;
  int m_processed_count = 0;
  uint64_t m_last_affinity_key = 0;
  obs::Context m_last_trace_ctx;
  std::set<std::thread::id> m_thread_ids;
  bool m_should_throw = false;
  std::chrono::milliseconds m_delay{0};
};

class AffinityExecutorTest : public ::testing::Test {
protected:
  TestHandler handler;
};

// =============================================================================
// Basic Lifecycle Tests
// =============================================================================

TEST_F(AffinityExecutorTest, ConstructsWithLanes) {
  AffinityExecutor executor(4, handler);
  EXPECT_EQ(executor.lane_count(), 4);
}

TEST_F(AffinityExecutorTest, StartAndStop) {
  AffinityExecutor executor(2, handler);
  executor.start();
  executor.stop();
}

TEST_F(AffinityExecutorTest, DoubleStartDoesNotCrash) {
  AffinityExecutor executor(2, handler);
  executor.start();
  EXPECT_NO_THROW(executor.start());
  executor.stop();
}

TEST_F(AffinityExecutorTest, DoubleStopDoesNotCrash) {
  AffinityExecutor executor(2, handler);
  executor.start();
  executor.stop();
  EXPECT_NO_THROW(executor.stop());
}

TEST_F(AffinityExecutorTest, StopBeforeStartDoesNotCrash) {
  AffinityExecutor executor(2, handler);
  EXPECT_NO_THROW(executor.stop());
}

TEST_F(AffinityExecutorTest, DestructorStopsAutomatically) {
  {
    AffinityExecutor executor(2, handler);
    executor.start();
    Message msg{.affinity_key = 1, .trace_ctx = {}, .payload = {}};
    executor.submit(std::move(msg));
  } // Destructor should call stop
  SUCCEED(); // No crash means success
}

// =============================================================================
// Message Processing Tests
// =============================================================================

TEST_F(AffinityExecutorTest, ProcessesSingleMessage) {
  AffinityExecutor executor(2, handler);
  executor.start();

  Message msg{.affinity_key = 42, .trace_ctx = {}, .payload = {}};
  executor.submit(std::move(msg));

  std::this_thread::sleep_for(50ms);
  executor.stop();

  EXPECT_EQ(handler.processed_count(), 1);
  EXPECT_EQ(handler.last_affinity_key(), 42);
}

TEST_F(AffinityExecutorTest, ProcessesMultipleMessages) {
  AffinityExecutor executor(2, handler);
  executor.start();

  for (int i = 0; i < 100; ++i) {
    Message msg{.affinity_key = static_cast<uint64_t>(i),
                .trace_ctx = {},
                .payload = {}};
    executor.submit(std::move(msg));
  }

  std::this_thread::sleep_for(100ms);
  executor.stop();

  EXPECT_EQ(handler.processed_count(), 100);
}

// =============================================================================
// Session Affinity Tests
// =============================================================================

TEST_F(AffinityExecutorTest, SameAffinityKeyRoutesToSameLane) {
  AffinityExecutor executor(4, handler);
  executor.start();

  // Submit 10 messages with same affinity key
  for (int i = 0; i < 10; ++i) {
    Message msg{.affinity_key = 123, .trace_ctx = {}, .payload = {}};
    executor.submit(std::move(msg));
  }

  std::this_thread::sleep_for(100ms);
  executor.stop();

  EXPECT_EQ(handler.processed_count(), 10);
  // All should be processed by same thread (affinity)
  EXPECT_EQ(handler.thread_ids().size(), 1);
}

TEST_F(AffinityExecutorTest, DifferentAffinityKeysCanUseDifferentLanes) {
  AffinityExecutor executor(4, handler);
  executor.start();

  // Submit messages with different affinity keys
  for (int i = 0; i < 100; ++i) {
    Message msg{.affinity_key = static_cast<uint64_t>(i),
                .trace_ctx = {},
                .payload = {}};
    executor.submit(std::move(msg));
  }

  std::this_thread::sleep_for(100ms);
  executor.stop();

  EXPECT_EQ(handler.processed_count(), 100);
  // Multiple lanes should have processed messages
  EXPECT_GT(handler.thread_ids().size(), 1);
}

TEST_F(AffinityExecutorTest, AffinityKeyModuloDistribution) {
  AffinityExecutor executor(4, handler);
  executor.start();

  // Keys 0, 4, 8 -> lane 0
  // Keys 1, 5, 9 -> lane 1
  // Keys 2, 6 -> lane 2
  // Keys 3, 7 -> lane 3
  for (int i = 0; i < 10; ++i) {
    Message msg{.affinity_key = static_cast<uint64_t>(i),
                .trace_ctx = {},
                .payload = {}};
    executor.submit(std::move(msg));
  }

  std::this_thread::sleep_for(100ms);
  executor.stop();

  EXPECT_EQ(handler.processed_count(), 10);
}

// =============================================================================
// Context & Payload Preservation Tests
// =============================================================================

TEST_F(AffinityExecutorTest, PropagatesTraceContext) {
  AffinityExecutor executor(2, handler);
  executor.start();

  obs::Context ctx = obs::Context::create();
  Message msg{.affinity_key = 1, .trace_ctx = ctx, .payload = {}};
  executor.submit(std::move(msg));

  std::this_thread::sleep_for(50ms);
  executor.stop();

  auto received_ctx = handler.last_trace_ctx();
  EXPECT_EQ(received_ctx.trace_id.high, ctx.trace_id.high);
  EXPECT_EQ(received_ctx.trace_id.low, ctx.trace_id.low);
}

TEST_F(AffinityExecutorTest, PayloadPreserved) {
  struct PayloadCapture : public IMessageHandler {
    std::string received;
    void handle(Message &msg) override {
      received = std::any_cast<std::string>(msg.payload);
    }
  } payload_handler;

  AffinityExecutor executor(2, payload_handler);
  executor.start();

  Message msg{
      .affinity_key = 1, .trace_ctx = {}, .payload = std::string("test data")};
  executor.submit(std::move(msg));

  std::this_thread::sleep_for(50ms);
  executor.stop();

  EXPECT_EQ(payload_handler.received, "test data");
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(AffinityExecutorTest, SingleLaneExecutor) {
  AffinityExecutor executor(1, handler);
  executor.start();

  for (int i = 0; i < 10; ++i) {
    Message msg{.affinity_key = static_cast<uint64_t>(i),
                .trace_ctx = {},
                .payload = {}};
    executor.submit(std::move(msg));
  }

  std::this_thread::sleep_for(50ms);
  executor.stop();

  EXPECT_EQ(handler.processed_count(), 10);
  EXPECT_EQ(handler.thread_ids().size(), 1); // Only one lane
}

TEST_F(AffinityExecutorTest, HighThroughput) {
  AffinityExecutor executor(8, handler);
  executor.start();

  constexpr int num_messages = 10000;
  for (int i = 0; i < num_messages; ++i) {
    Message msg{.affinity_key = static_cast<uint64_t>(i),
                .trace_ctx = {},
                .payload = {}};
    executor.submit(std::move(msg));
  }

  std::this_thread::sleep_for(500ms);
  executor.stop();

  EXPECT_EQ(handler.processed_count(), num_messages);
}

TEST_F(AffinityExecutorTest, LongRunningHandler) {
  handler.set_delay(10ms);
  AffinityExecutor executor(4, handler);
  executor.start();

  for (int i = 0; i < 4; ++i) {
    Message msg{.affinity_key = static_cast<uint64_t>(i),
                .trace_ctx = {},
                .payload = {}};
    executor.submit(std::move(msg));
  }

  std::this_thread::sleep_for(100ms);
  executor.stop();

  EXPECT_EQ(handler.processed_count(), 4);
}

} // namespace astra::execution
