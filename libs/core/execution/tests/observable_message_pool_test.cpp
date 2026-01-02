#include "IMessageHandler.h"
#include "Message.h"
#include "ObservableMessagePool.h"
#include "StickyQueue.h"

#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

using namespace astra::execution;
using namespace std::chrono_literals;

/**
 * TDD Tests for ObservableMessagePool
 */

class MockHandler : public IMessageHandler {
public:
  void handle(Message &msg) override {
    m_count++;
    if (m_delay_ms > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(m_delay_ms));
    }
  }

  std::atomic<int> m_count{0};
  int m_delay_ms{0};
};

class ObservableMessagePoolTest : public ::testing::Test {
protected:
  MockHandler handler;
};

TEST_F(ObservableMessagePoolTest, SubmitIncrementsCounter) {
  StickyQueue core(2, handler);
  ObservableMessagePool pool(core);

  pool.start();

  Message msg{1, obs::Context{}, std::any{}};
  pool.submit(std::move(msg));

  std::this_thread::sleep_for(50ms);
  pool.stop();

  EXPECT_EQ(handler.m_count, 1);
}

TEST_F(ObservableMessagePoolTest, DelegatesStartStop) {
  StickyQueue core(2, handler);
  ObservableMessagePool pool(core);

  pool.start();

  for (int i = 0; i < 5; ++i) {
    Message msg{static_cast<uint64_t>(i), obs::Context{}, std::any{}};
    pool.submit(std::move(msg));
  }

  std::this_thread::sleep_for(100ms);
  pool.stop();

  EXPECT_EQ(handler.m_count, 5);
}

TEST_F(ObservableMessagePoolTest, HandlerWrapperDecrementsQueueDepth) {
  // Test that ObservableHandlerWrapper works correctly
  auto depth = obs::gauge("test.queue_depth", obs::Unit::Dimensionless);

  // Pass by value (copy)
  ObservableHandlerWrapper wrapper(handler, depth);

  // Simulate: submit 3 messages (inc queue_depth 3 times)
  depth.add(1);
  depth.add(1);
  depth.add(1);

  // Handle messages (dec queue_depth)
  Message msg1{1, obs::Context{}, std::any{}};
  Message msg2{2, obs::Context{}, std::any{}};
  Message msg3{3, obs::Context{}, std::any{}};

  wrapper.handle(msg1);
  wrapper.handle(msg2);
  wrapper.handle(msg3);

  EXPECT_EQ(handler.m_count, 3);
}

TEST_F(ObservableMessagePoolTest, WorkerCountDelegates) {
  StickyQueue core(8, handler);
  ObservableMessagePool pool(core);

  EXPECT_EQ(pool.worker_count(), 8);
}
