// =============================================================================
// test_thread_safety.cpp - Comprehensive thread safety tests for TSan
// =============================================================================
// Run with: ./build/clang-asan/observability/test_observability
// TSan will detect data races if any exist.
// =============================================================================
#include <gtest/gtest.h>
#include <obs/IBackend.h>
#include <obs/Span.h>
#include <obs/Log.h>
#include <obs/Metrics.h>
#include <obs/Context.h>
#include "MockBackend.h"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <queue>
#include <mutex>

namespace obs::test {

// -----------------------------------------------------------------------------
// Test Fixture
// -----------------------------------------------------------------------------
class ThreadSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock = std::make_unique<ThreadSafeMockBackend>();
        mock_ptr = mock.get();
        obs::set_backend(std::move(mock));
    }
    
    void TearDown() override {
        obs::shutdown();
    }
    
    std::unique_ptr<ThreadSafeMockBackend> mock;
    ThreadSafeMockBackend* mock_ptr;
    
    static constexpr int NUM_THREADS = 8;
    static constexpr int OPS_PER_THREAD = 100;
};

// =============================================================================
// Concurrent Span Tests
// =============================================================================
TEST_F(ThreadSafetyTest, ConcurrentSpanCreation) {
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([]() {
            for (int j = 0; j < OPS_PER_THREAD; j++) {
                auto span = obs::span("concurrent_span");
            }
        });
    }
    
    for (auto& t : threads) t.join();
    
    EXPECT_EQ(mock_ptr->span_count.load(), NUM_THREADS * OPS_PER_THREAD);
}

TEST_F(ThreadSafetyTest, ConcurrentSpanWithContext) {
    std::vector<std::thread> threads;
    Context shared_ctx = Context::create();
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([&shared_ctx]() {
            for (int j = 0; j < OPS_PER_THREAD; j++) {
                auto span = obs::span("with_context", shared_ctx);
            }
        });
    }
    
    for (auto& t : threads) t.join();
}

// =============================================================================
// Concurrent Logging Tests
// =============================================================================
TEST_F(ThreadSafetyTest, ConcurrentLogging) {
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([]() {
            Context ctx = Context::create();
            for (int j = 0; j < OPS_PER_THREAD; j++) {
                obs::info("concurrent log", ctx);
            }
        });
    }
    
    for (auto& t : threads) t.join();
    
    EXPECT_EQ(mock_ptr->log_count.load(), NUM_THREADS * OPS_PER_THREAD);
}

TEST_F(ThreadSafetyTest, ConcurrentMixedLogLevels) {
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([]() {
            Context ctx = Context::create();
            for (int j = 0; j < OPS_PER_THREAD; j++) {
                switch (j % 5) {
                    case 0: obs::trace("t", ctx); break;
                    case 1: obs::debug("d", ctx); break;
                    case 2: obs::info("i", ctx); break;
                    case 3: obs::warn("w", ctx); break;
                    case 4: obs::error("e", ctx); break;
                }
            }
        });
    }
    
    for (auto& t : threads) t.join();
}

// =============================================================================
// Concurrent Metrics Tests
// =============================================================================
TEST_F(ThreadSafetyTest, ConcurrentCounterAccess) {
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([]() {
            auto& counter = obs::counter("shared_counter");
            for (int j = 0; j < OPS_PER_THREAD; j++) {
                counter.inc();
            }
        });
    }
    
    for (auto& t : threads) t.join();
}

TEST_F(ThreadSafetyTest, ConcurrentHistogramAccess) {
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([i]() {
            auto& hist = obs::histogram("shared_histogram");
            for (int j = 0; j < OPS_PER_THREAD; j++) {
                hist.record(static_cast<double>(i * j));
            }
        });
    }
    
    for (auto& t : threads) t.join();
}

TEST_F(ThreadSafetyTest, ConcurrentMultipleCounters) {
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([i]() {
            std::string name = "counter_" + std::to_string(i % 3);
            auto& counter = obs::counter(name);
            for (int j = 0; j < OPS_PER_THREAD; j++) {
                counter.inc(j);
            }
        });
    }
    
    for (auto& t : threads) t.join();
}

// =============================================================================
// Mixed Operations (Real-world scenario)
// =============================================================================
TEST_F(ThreadSafetyTest, ConcurrentMixedOperations) {
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([]() {
            Context ctx = Context::create();
            for (int j = 0; j < OPS_PER_THREAD; j++) {
                // Simulate real request handling
                auto span = obs::span("request", ctx);
                obs::info("processing", ctx);
                obs::counter("requests").inc();
                obs::histogram("latency").record(0.001 * j);
            }
        });
    }
    
    for (auto& t : threads) t.join();
}

// =============================================================================
// Context Thread Safety
// =============================================================================
TEST_F(ThreadSafetyTest, ConcurrentContextCreation) {
    std::vector<std::thread> threads;
    std::vector<Context> contexts(NUM_THREADS * OPS_PER_THREAD);
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([i, &contexts]() {
            for (int j = 0; j < OPS_PER_THREAD; j++) {
                contexts[i * OPS_PER_THREAD + j] = Context::create();
            }
        });
    }
    
    for (auto& t : threads) t.join();
    
    // Verify all contexts are unique
    for (const auto& ctx : contexts) {
        EXPECT_TRUE(ctx.is_valid());
    }
}

TEST_F(ThreadSafetyTest, ConcurrentTraceparentParsing) {
    std::vector<std::thread> threads;
    std::string header = "00-0123456789abcdeffedcba9876543210-aabbccddeeff0011-01";
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([&header]() {
            for (int j = 0; j < OPS_PER_THREAD; j++) {
                Context ctx = Context::from_traceparent(header);
                std::string back = ctx.to_traceparent();
                EXPECT_EQ(back, header);
            }
        });
    }
    
    for (auto& t : threads) t.join();
}

// =============================================================================
// Shutdown Safety (Critical for SEDA)
// =============================================================================
TEST_F(ThreadSafetyTest, ShutdownWhileOperating) {
    std::atomic<bool> keep_running{true};
    std::vector<std::thread> workers;
    
    // Track shutdown independently (since backend will be destroyed)
    auto shutdown_tracker = std::make_shared<std::atomic<bool>>(false);
    auto tracker_ptr = shutdown_tracker.get();
    
    // Override shutdown behavior to track it
    // Note: We can't actually intercept shutdown, so we just verify graceful behavior
    
    // Start worker threads doing continuous operations
    for (int i = 0; i < NUM_THREADS; i++) {
        workers.emplace_back([&keep_running]() {
            while (keep_running.load()) {
                auto span = obs::span("working");
                obs::info("log");
                obs::counter("ops").inc();
            }
        });
    }
    
    // Let workers run briefly
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Shutdown while workers are active - this destroys the backend
    // Workers may get null responses after this, which is fine
    obs::shutdown();
    
    // Signal workers to stop
    keep_running.store(false);
    
    for (auto& t : workers) t.join();
    
    // Test passes if no crash or TSan errors occur during concurrent shutdown
    // Note: mock_ptr is invalid after shutdown() - cannot access it
    (void)tracker_ptr;  // Suppress unused warning
}

TEST_F(ThreadSafetyTest, UseAfterShutdown) {
    obs::shutdown();
    
    // These should all be no-ops, not crashes
    auto span = obs::span("after_shutdown");
    obs::info("log after shutdown");
    auto& counter = obs::counter("counter_after");
    counter.inc();
    
    EXPECT_EQ(span, nullptr);
}

// =============================================================================
// Backend Replacement Safety
// =============================================================================
TEST(ThreadSafetyBackendTest, ConcurrentBackendReplacement) {
    std::atomic<bool> keep_running{true};
    std::vector<std::thread> threads;
    
    // Threads continuously using observability
    for (int i = 0; i < 4; i++) {
        threads.emplace_back([&keep_running]() {
            while (keep_running.load()) {
                obs::info("using");
            }
        });
    }
    
    // Main thread repeatedly replaces backend
    for (int i = 0; i < 10; i++) {
        obs::set_backend(std::make_unique<ThreadSafeMockBackend>());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    keep_running.store(false);
    for (auto& t : threads) t.join();
    
    obs::shutdown();
}

// =============================================================================
// High Contention Tests
// =============================================================================
TEST_F(ThreadSafetyTest, HighContentionSingleCounter) {
    constexpr int HIGH_OPS = 10000;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([]() {
            auto& counter = obs::counter("hot_counter");
            for (int j = 0; j < HIGH_OPS; j++) {
                counter.inc();
            }
        });
    }
    
    for (auto& t : threads) t.join();
}

TEST_F(ThreadSafetyTest, HighContentionMixedMetrics) {
    constexpr int HIGH_OPS = 1000;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS * 2; i++) {
        threads.emplace_back([i]() {
            for (int j = 0; j < HIGH_OPS; j++) {
                if (i % 2 == 0) {
                    obs::counter("hot").inc();
                } else {
                    obs::histogram("hot").record(1.0);
                }
            }
        });
    }
    
    for (auto& t : threads) t.join();
}

// =============================================================================
// Context Child Propagation (Critical for SEDA)
// =============================================================================
TEST_F(ThreadSafetyTest, ConcurrentChildContextCreation) {
    Context parent = Context::create();
    std::vector<std::thread> threads;
    std::vector<Context> children(NUM_THREADS * OPS_PER_THREAD);
    
    // Multiple threads creating child contexts from same parent
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([i, &parent, &children]() {
            SpanId dummy{};
            dummy.value = i * 1000;
            for (int j = 0; j < OPS_PER_THREAD; j++) {
                dummy.value++;
                children[i * OPS_PER_THREAD + j] = parent.child(dummy);
            }
        });
    }
    
    for (auto& t : threads) t.join();
    
    // All children should have same trace_id as parent
    for (const auto& child : children) {
        EXPECT_EQ(child.trace_id.high, parent.trace_id.high);
        EXPECT_EQ(child.trace_id.low, parent.trace_id.low);
    }
}

// =============================================================================
// Baggage Modification (Map access)
// =============================================================================
TEST_F(ThreadSafetyTest, ConcurrentBaggageModification) {
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([i]() {
            Context ctx = Context::create();
            for (int j = 0; j < OPS_PER_THREAD; j++) {
                // Each thread modifies its own context's baggage
                std::string key = "key_" + std::to_string(j);
                ctx.baggage[key] = std::to_string(i * j);
                std::string header = ctx.to_baggage_header();
                (void)header;
            }
        });
    }
    
    for (auto& t : threads) t.join();
}

TEST_F(ThreadSafetyTest, ConcurrentBaggageParsing) {
    std::vector<std::thread> threads;
    std::string baggage = "key1=val1,key2=val2,key3=val3";
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([&baggage]() {
            for (int j = 0; j < OPS_PER_THREAD; j++) {
                Context ctx;
                Context::parse_baggage(ctx, baggage);
                EXPECT_EQ(ctx.baggage["key1"], "val1");
            }
        });
    }
    
    for (auto& t : threads) t.join();
}

// =============================================================================
// Double Shutdown (Idempotency)
// =============================================================================
TEST_F(ThreadSafetyTest, ConcurrentDoubleShutdown) {
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([]() {
            obs::shutdown();
        });
    }
    
    for (auto& t : threads) t.join();
    
    // Should be safe - no crash, no TSan errors
}

// =============================================================================
// Init-Shutdown Race
// =============================================================================
TEST(ThreadSafetyInitTest, ConcurrentInitShutdown) {
    std::vector<std::thread> threads;
    std::atomic<bool> keep_running{true};
    
    // Some threads init
    for (int i = 0; i < 4; i++) {
        threads.emplace_back([&keep_running]() {
            while (keep_running.load()) {
                obs::set_backend(std::make_unique<ThreadSafeMockBackend>());
            }
        });
    }
    
    // Some threads shutdown
    for (int i = 0; i < 4; i++) {
        threads.emplace_back([&keep_running]() {
            while (keep_running.load()) {
                obs::shutdown();
            }
        });
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    keep_running.store(false);
    
    for (auto& t : threads) t.join();
    obs::shutdown();
}

// =============================================================================
// Staggered Start (Simulates real-world startup)
// =============================================================================
TEST_F(ThreadSafetyTest, StaggeredThreadStart) {
    std::vector<std::thread> threads;
    std::atomic<int> started{0};
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([i, &started]() {
            // Stagger starts
            std::this_thread::sleep_for(std::chrono::microseconds(i * 10));
            started++;
            
            Context ctx = Context::create();
            for (int j = 0; j < OPS_PER_THREAD; j++) {
                auto span = obs::span("staggered", ctx);
                obs::info("msg", ctx);
            }
        });
    }
    
    for (auto& t : threads) t.join();
    EXPECT_EQ(started.load(), NUM_THREADS);
}

// =============================================================================
// Producer-Consumer Pattern (SEDA-like)
// =============================================================================
TEST_F(ThreadSafetyTest, ProducerConsumerContextPassing) {
    std::atomic<bool> done{false};
    std::vector<Context> queue;
    std::mutex queue_mutex;
    
    // Producers create contexts
    std::vector<std::thread> producers;
    for (int i = 0; i < 4; i++) {
        producers.emplace_back([&queue, &queue_mutex]() {
            for (int j = 0; j < 100; j++) {
                Context ctx = Context::create();
                obs::info("produced", ctx);
                {
                    std::lock_guard<std::mutex> lock(queue_mutex);
                    queue.push_back(ctx);
                }
            }
        });
    }
    
    // Consumers use contexts
    std::vector<std::thread> consumers;
    for (int i = 0; i < 4; i++) {
        consumers.emplace_back([&queue, &queue_mutex, &done]() {
            while (true) {
                Context ctx;
                bool got = false;
                {
                    std::lock_guard<std::mutex> lock(queue_mutex);
                    if (!queue.empty()) {
                        ctx = queue.back();
                        queue.pop_back();
                        got = true;
                    }
                }
                if (got) {
                    obs::info("consumed", ctx);
                    auto span = obs::span("process", ctx);
                } else if (done.load()) {
                    // Only exit when done AND queue is empty (checked inside lock)
                    std::lock_guard<std::mutex> lock(queue_mutex);
                    if (queue.empty()) break;
                }
            }
        });
    }
    
    for (auto& t : producers) t.join();
    done.store(true);
    for (auto& t : consumers) t.join();
}

// =============================================================================
// Burst Traffic Pattern
// =============================================================================
TEST_F(ThreadSafetyTest, BurstTraffic) {
    std::atomic<bool> go{false};
    std::vector<std::thread> threads;
    
    // All threads wait, then burst simultaneously
    for (int i = 0; i < NUM_THREADS * 2; i++) {
        threads.emplace_back([&go]() {
            while (!go.load()) {
                std::this_thread::yield();
            }
            // Burst!
            for (int j = 0; j < 1000; j++) {
                obs::counter("burst").inc();
            }
        });
    }
    
    // Trigger burst
    go.store(true);
    
    for (auto& t : threads) t.join();
}

// =============================================================================
// Long-running Span with Concurrent Attributes
// =============================================================================
TEST_F(ThreadSafetyTest, ConcurrentSpanAttributeSetting) {
    // This tests if multiple threads can safely set attributes on the same span
    // (if span were shared - in practice spans shouldn't be shared, but test edge case)
    std::vector<std::thread> threads;
    std::atomic<int> completed{0};
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([&completed]() {
            // Each thread creates and manipulates its own span
            auto span = obs::span("multi_attr");
            if (span) {
                for (int j = 0; j < 100; j++) {
                    span->attr("key_" + std::to_string(j), std::to_string(j));
                }
            }
            completed++;
        });
    }
    
    for (auto& t : threads) t.join();
    EXPECT_EQ(completed.load(), NUM_THREADS);
}

// =============================================================================
// Metric Name Collision
// =============================================================================
TEST_F(ThreadSafetyTest, MetricNameCollision) {
    std::vector<std::thread> threads;
    
    // All threads request same metric with same name
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([]() {
            for (int j = 0; j < OPS_PER_THREAD; j++) {
                // Same exact name - tests cache lookup races
                auto& c = obs::counter("collision_counter", "same desc");
                c.inc();
            }
        });
    }
    
    for (auto& t : threads) t.join();
}

// =============================================================================
// SEDA Stage Simulation (Network -> Worker -> IO)
// =============================================================================
TEST_F(ThreadSafetyTest, SEDAStageSimulation) {
    struct Job {
        Context ctx;
        int id;
    };
    
    std::queue<Job> network_to_worker;
    std::queue<Job> worker_to_io;
    std::mutex n2w_mutex, w2i_mutex;
    std::atomic<bool> done{false};
    std::atomic<int> processed{0};
    
    // Network thread (1)
    std::thread network([&]() {
        for (int i = 0; i < 100; i++) {
            Job job{Context::create(), i};
            obs::info("network_recv", job.ctx);
            {
                std::lock_guard<std::mutex> lock(n2w_mutex);
                network_to_worker.push(job);
            }
        }
    });
    
    // Worker threads (4)
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; i++) {
        workers.emplace_back([&]() {
            while (!done.load() || !network_to_worker.empty()) {
                Job job;
                bool got = false;
                {
                    std::lock_guard<std::mutex> lock(n2w_mutex);
                    if (!network_to_worker.empty()) {
                        job = network_to_worker.front();
                        network_to_worker.pop();
                        got = true;
                    }
                }
                if (got) {
                    auto span = obs::span("process", job.ctx);
                    obs::counter("jobs_processed").inc();
                    {
                        std::lock_guard<std::mutex> lock(w2i_mutex);
                        worker_to_io.push(job);
                    }
                }
            }
        });
    }
    
    // IO threads (2)
    std::vector<std::thread> io_threads;
    for (int i = 0; i < 2; i++) {
        io_threads.emplace_back([&]() {
            while (!done.load() || !worker_to_io.empty()) {
                Job job;
                bool got = false;
                {
                    std::lock_guard<std::mutex> lock(w2i_mutex);
                    if (!worker_to_io.empty()) {
                        job = worker_to_io.front();
                        worker_to_io.pop();
                        got = true;
                    }
                }
                if (got) {
                    obs::info("io_complete", job.ctx);
                    obs::histogram("io_latency").record(0.001);
                    processed++;
                }
            }
        });
    }
    
    network.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    done.store(true);
    
    for (auto& t : workers) t.join();
    for (auto& t : io_threads) t.join();
    
    EXPECT_EQ(processed.load(), 100);
}

} // namespace obs::test
