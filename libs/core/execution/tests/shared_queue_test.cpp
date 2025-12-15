#include "SharedQueue.h"
#include <Context.h>
#include <gtest/gtest.h>
#include <vector>
#include <atomic>
#include <chrono>
#include <functional>
#include <thread>
#include <mutex>
#include <set>

using namespace astra::execution;
using namespace std::chrono_literals;

// =============================================================================
// Basic Lifecycle Tests
// =============================================================================

class SharedQueueTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(SharedQueueTest, StartStop) {
    SharedQueue queue(2);
    queue.start();
    queue.stop();
}

TEST_F(SharedQueueTest, DoubleStartDoesNotCrash) {
    SharedQueue queue(2);
    queue.start();
    EXPECT_NO_THROW(queue.start());
    queue.stop();
}

TEST_F(SharedQueueTest, DoubleStopDoesNotCrash) {
    SharedQueue queue(2);
    queue.start();
    queue.stop();
    EXPECT_NO_THROW(queue.stop());
}

TEST_F(SharedQueueTest, StopBeforeStartDoesNotCrash) {
    SharedQueue queue(2);
    EXPECT_NO_THROW(queue.stop());
}

TEST_F(SharedQueueTest, DestructorStopsCleanly) {
    {
        SharedQueue queue(2);
        queue.start();
        queue.submit({1, obs::Context{}, std::function<void()>([]{ std::this_thread::sleep_for(10ms); })});
        // Destructor should wait for workers
    }
    SUCCEED();
}

// =============================================================================
// Submit Tests
// =============================================================================

TEST_F(SharedQueueTest, SubmitMessage) {
    SharedQueue queue(2);
    queue.start();
    
    bool result = queue.submit({1, obs::Context{}, std::function<void()>([]{})});
    EXPECT_TRUE(result);
    
    queue.stop();
}

TEST_F(SharedQueueTest, SubmitBeforeStart) {
    SharedQueue queue(2);
    
    // Submitting before start - behavior is implementation defined
    // Should not crash
    EXPECT_NO_THROW({
        queue.submit({1, obs::Context{}, std::function<void()>([]{})});
    });
}

TEST_F(SharedQueueTest, SubmitAfterStop) {
    SharedQueue queue(2);
    queue.start();
    queue.stop();
    
    // Submitting after stop should return false (rejected)
    bool result = queue.submit({1, obs::Context{}, std::function<void()>([]{})});
    EXPECT_FALSE(result);
}

// =============================================================================
// Backpressure Tests
// =============================================================================

TEST_F(SharedQueueTest, Backpressure) {
    SharedQueue queue(0, 2);  // 0 workers to prevent consumption, max 2 messages
    queue.start();
    
    EXPECT_TRUE(queue.submit({1, obs::Context{}, std::function<void()>([]{})  }));
    EXPECT_TRUE(queue.submit({2, obs::Context{}, std::function<void()>([]{})  }));
    EXPECT_FALSE(queue.submit({3, obs::Context{}, std::function<void()>([]{})  }));  // Rejected
    
    queue.stop();
}

TEST_F(SharedQueueTest, BackpressureWithLargeLimit) {
    SharedQueue queue(1, 1000);
    queue.start();
    
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(queue.submit({(uint64_t)i, obs::Context{}, std::function<void()>([]{})  }));
    }
    
    queue.stop();
}

// =============================================================================
// Message Processing Tests
// =============================================================================

TEST_F(SharedQueueTest, MessagePayloadIsExecuted) {
    std::atomic<int> counter{0};
    
    SharedQueue queue(2);
    queue.start();
    
    for (int i = 0; i < 10; ++i) {
        queue.submit({(uint64_t)i, obs::Context{}, std::function<void()>([&counter]{ 
            counter++; 
        })});
    }
    
    std::this_thread::sleep_for(100ms);  // Wait for processing
    queue.stop();
    
    EXPECT_EQ(counter.load(), 10);
}

TEST_F(SharedQueueTest, LongRunningPayload) {
    std::atomic<int> counter{0};
    
    SharedQueue queue(2);
    queue.start();
    
    queue.submit({1, obs::Context{}, std::function<void()>([&counter]{ 
        std::this_thread::sleep_for(50ms);
        counter++; 
    })});
    
    queue.stop();  // Should wait for completion
    
    EXPECT_EQ(counter.load(), 1);
}

// =============================================================================
// Concurrency Tests
// =============================================================================

TEST_F(SharedQueueTest, ConcurrentSubmission) {
    std::atomic<int> processed{0};
    SharedQueue queue(4);
    queue.start();
    
    std::vector<std::thread> submitters;
    for (int i = 0; i < 10; ++i) {
        submitters.emplace_back([&queue, &processed]() {
            for (int j = 0; j < 100; ++j) {
                queue.submit({(uint64_t)j, obs::Context{}, std::function<void()>([&processed]{ 
                    processed++; 
                })});
            }
        });
    }
    
    for (auto& t : submitters) {
        t.join();
    }
    
    std::this_thread::sleep_for(200ms);  // Wait for processing
    queue.stop();
    
    EXPECT_EQ(processed.load(), 1000);
}

TEST_F(SharedQueueTest, NoSessionAffinity) {
    // Verify that messages are processed by multiple workers (no stickiness)
    std::mutex worker_mutex;
    std::set<std::thread::id> worker_ids;
    
    SharedQueue queue(4);
    queue.start();
    
    for (int i = 0; i < 100; ++i) {
        queue.submit({(uint64_t)i, obs::Context{}, std::function<void()>([&]{ 
            std::lock_guard<std::mutex> lock(worker_mutex);
            worker_ids.insert(std::this_thread::get_id());
            std::this_thread::sleep_for(1ms);  // Slow down to allow distribution
        })});
    }
    
    std::this_thread::sleep_for(500ms);
    queue.stop();
    
    // Should have used multiple workers
    EXPECT_GT(worker_ids.size(), 1);
}

// =============================================================================
// Worker Count Tests
// =============================================================================

TEST_F(SharedQueueTest, SingleWorker) {
    std::atomic<int> counter{0};
    
    SharedQueue queue(1);
    queue.start();
    
    for (int i = 0; i < 10; ++i) {
        queue.submit({(uint64_t)i, obs::Context{}, std::function<void()>([&counter]{ counter++; })});
    }
    
    std::this_thread::sleep_for(100ms);
    queue.stop();
    
    EXPECT_EQ(counter.load(), 10);
}

TEST_F(SharedQueueTest, ManyWorkers) {
    std::atomic<int> counter{0};
    
    SharedQueue queue(16);
    queue.start();
    
    for (int i = 0; i < 100; ++i) {
        queue.submit({(uint64_t)i, obs::Context{}, std::function<void()>([&counter]{ counter++; })});
    }
    
    std::this_thread::sleep_for(100ms);
    queue.stop();
    
    EXPECT_EQ(counter.load(), 100);
}

// =============================================================================
// Context Propagation Tests
// =============================================================================

TEST_F(SharedQueueTest, ContextIsPassedToMessage) {
    // Verify that context can be accessed in message
    SharedQueue queue(1);
    queue.start();
    
    obs::Context ctx;  // Create a context
    
    std::atomic<bool> context_received{false};
    queue.submit({1, ctx, std::function<void()>([&context_received]{ 
        context_received = true; 
    })});
    
    std::this_thread::sleep_for(50ms);
    queue.stop();
    
    EXPECT_TRUE(context_received);
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST_F(SharedQueueTest, StressHighVolume) {
    std::atomic<int> counter{0};
    
    SharedQueue queue(8);
    queue.start();
    
    for (int i = 0; i < 10000; ++i) {
        queue.submit({(uint64_t)i, obs::Context{}, std::function<void()>([&counter]{ counter++; })});
    }
    
    std::this_thread::sleep_for(500ms);
    queue.stop();
    
    EXPECT_EQ(counter.load(), 10000);
}

TEST_F(SharedQueueTest, GracefulShutdownUnderLoad) {
    std::atomic<int> counter{0};
    
    SharedQueue queue(4);
    queue.start();
    
    // Submit many slow tasks
    for (int i = 0; i < 50; ++i) {
        queue.submit({(uint64_t)i, obs::Context{}, std::function<void()>([&counter]{ 
            std::this_thread::sleep_for(10ms);
            counter++; 
        })});
    }
    
    // Stop immediately - should wait for in-flight tasks
    queue.stop();
    
    // All submitted tasks should complete
    EXPECT_EQ(counter.load(), 50);
}

// =============================================================================
// Proto Config Tests
// =============================================================================

TEST_F(SharedQueueTest, ConstructFromProtoConfig) {
    ::execution::SharedQueueConfig config;
    config.set_num_workers(4);
    config.set_max_queue_size(1000);
    
    SharedQueue queue(config);
    queue.start();
    
    std::atomic<int> counter{0};
    queue.submit({1, obs::Context{}, std::function<void()>([&counter]{ counter++; })});
    
    std::this_thread::sleep_for(50ms);
    queue.stop();
    
    EXPECT_EQ(counter.load(), 1);
}
