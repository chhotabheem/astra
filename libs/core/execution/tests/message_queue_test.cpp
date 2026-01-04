#include "MessageQueue.h"

#include <atomic>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

namespace astra::execution {

using namespace std::chrono_literals;

// =============================================================================
// Basic Operations
// =============================================================================

TEST(MessageQueueTest, PushAndPop) {
  MessageQueue queue;

  Message msg{.affinity_key = 42, .trace_ctx = {}, .payload = 123};
  queue.push(std::move(msg));

  auto result = queue.pop();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->affinity_key, 42);
}

TEST(MessageQueueTest, PopBlocksUntilMessage) {
  MessageQueue queue;
  std::atomic<bool> popped{false};

  std::thread consumer([&]() {
    auto result = queue.pop();
    popped.store(result.has_value());
  });

  std::this_thread::sleep_for(20ms);
  EXPECT_FALSE(popped.load()); // Should still be blocked

  queue.push(Message{.affinity_key = 1, .trace_ctx = {}, .payload = {}});
  consumer.join();

  EXPECT_TRUE(popped.load());
}

TEST(MessageQueueTest, CloseWakesBlockedPop) {
  MessageQueue queue;

  std::thread consumer([&]() {
    auto result = queue.pop();
    EXPECT_FALSE(result.has_value()); // Should return nullopt
  });

  std::this_thread::sleep_for(20ms);
  queue.close();
  consumer.join();
}

TEST(MessageQueueTest, PushAfterCloseIsIgnored) {
  MessageQueue queue;
  queue.close();

  queue.push(Message{.affinity_key = 1, .trace_ctx = {}, .payload = {}});

  auto result = queue.pop();
  EXPECT_FALSE(result.has_value());
}

TEST(MessageQueueTest, FIFOOrder) {
  MessageQueue queue;

  for (int i = 0; i < 5; ++i) {
    queue.push(Message{.affinity_key = static_cast<uint64_t>(i),
                       .trace_ctx = {},
                       .payload = {}});
  }

  for (int i = 0; i < 5; ++i) {
    auto result = queue.pop();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->affinity_key, static_cast<uint64_t>(i));
  }
}

// =============================================================================
// Concurrent Operations
// =============================================================================

TEST(MessageQueueTest, ConcurrentPushPopSafetyTest) {
  MessageQueue queue;
  constexpr int num_messages = 1000;
  std::atomic<int> received{0};

  // Producer thread
  std::thread producer([&]() {
    for (int i = 0; i < num_messages; ++i) {
      queue.push(Message{.affinity_key = static_cast<uint64_t>(i),
                         .trace_ctx = {},
                         .payload = i});
    }
    queue.close();
  });

  // Consumer thread
  std::thread consumer([&]() {
    while (auto msg = queue.pop()) {
      received.fetch_add(1);
    }
  });

  producer.join();
  consumer.join();

  EXPECT_EQ(received.load(), num_messages);
}

TEST(MessageQueueTest, MultipleConsumersSafetyTest) {
  MessageQueue queue;
  constexpr int num_messages = 1000;
  std::atomic<int> received{0};

  // Push all messages first
  for (int i = 0; i < num_messages; ++i) {
    queue.push(Message{.affinity_key = static_cast<uint64_t>(i),
                       .trace_ctx = {},
                       .payload = i});
  }

  // Start multiple consumers
  std::vector<std::thread> consumers;
  for (int i = 0; i < 4; ++i) {
    consumers.emplace_back([&]() {
      while (auto msg = queue.pop()) {
        received.fetch_add(1);
      }
    });
  }

  std::this_thread::sleep_for(50ms);
  queue.close();

  for (auto &t : consumers) {
    t.join();
  }

  EXPECT_EQ(received.load(), num_messages);
}

TEST(MessageQueueTest, MultipleProducersSafetyTest) {
  MessageQueue queue;
  constexpr int messages_per_producer = 250;
  constexpr int num_producers = 4;
  std::atomic<int> received{0};

  std::vector<std::thread> producers;
  for (int p = 0; p < num_producers; ++p) {
    producers.emplace_back([&, p]() {
      for (int i = 0; i < messages_per_producer; ++i) {
        queue.push(Message{.affinity_key = static_cast<uint64_t>(p * 1000 + i),
                           .trace_ctx = {},
                           .payload = i});
      }
    });
  }

  std::thread consumer([&]() {
    while (auto msg = queue.pop()) {
      received.fetch_add(1);
    }
  });

  for (auto &t : producers) {
    t.join();
  }

  std::this_thread::sleep_for(50ms);
  queue.close();
  consumer.join();

  EXPECT_EQ(received.load(), messages_per_producer * num_producers);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST(MessageQueueTest, PopFromEmptyQueueBlocks) {
  MessageQueue queue;
  std::atomic<bool> timed_out{false};

  std::thread consumer([&]() {
    auto start = std::chrono::steady_clock::now();
    // This should block until close is called
    auto result = queue.pop();
    auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed >= 40ms) {
      timed_out.store(true);
    }
  });

  std::this_thread::sleep_for(50ms);
  queue.close();
  consumer.join();

  EXPECT_TRUE(timed_out.load());
}

TEST(MessageQueueTest, CloseIsIdempotent) {
  MessageQueue queue;
  queue.close();
  EXPECT_NO_THROW(queue.close());
  EXPECT_NO_THROW(queue.close());
}

TEST(MessageQueueTest, PayloadPreserved) {
  MessageQueue queue;

  std::string payload = "test payload data";
  queue.push(Message{.affinity_key = 1, .trace_ctx = {}, .payload = payload});

  auto result = queue.pop();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(std::any_cast<std::string>(result->payload), payload);
}

TEST(MessageQueueTest, TraceContextPreserved) {
  MessageQueue queue;

  obs::Context ctx = obs::Context::create();
  queue.push(Message{.affinity_key = 1, .trace_ctx = ctx, .payload = {}});

  auto result = queue.pop();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->trace_ctx.trace_id.high, ctx.trace_id.high);
  EXPECT_EQ(result->trace_ctx.trace_id.low, ctx.trace_id.low);
}

} // namespace astra::execution
