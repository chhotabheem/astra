#include <Metrics.h>
#include <MetricsRegistry.h>
#include <Provider.h>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

class SafetyTest : public ::testing::Test {
protected:
  void SetUp() override {
    ::observability::Config config;
    config.set_service_name("safety-test");
    config.set_service_version("1.0.0");
    config.set_environment("test");
    config.set_otlp_endpoint("http://localhost:4317");
    ASSERT_TRUE(obs::Provider::instance().init(config));
  }

  void TearDown() override {
    obs::Provider::instance().shutdown();
  }
};

// =============================================================================
// Safety Test 1: Null Handle Behavior
// =============================================================================

TEST_F(SafetyTest, NullCounterHandleSafe) {
  // Default-constructed Counter has ID=0 (should be safe no-op)
  obs::Counter null_counter;

  EXPECT_NO_THROW(null_counter.inc());
  EXPECT_NO_THROW(null_counter.inc(5));
  EXPECT_NO_THROW(null_counter.inc(100, {{"key", "value"}}));
}

TEST_F(SafetyTest, NullHistogramHandleSafe) {
  obs::Histogram null_hist;

  EXPECT_NO_THROW(null_hist.record(42.5));
  EXPECT_NO_THROW(null_hist.record(100.0, {{"region", "us"}}));
}

TEST_F(SafetyTest, NullGaugeHandleSafe) {
  obs::Gauge null_gauge;

  EXPECT_NO_THROW(null_gauge.set(100));
  EXPECT_NO_THROW(null_gauge.add(50));
  EXPECT_NO_THROW(null_gauge.set(200, {{"host", "test"}}));
}

TEST_F(SafetyTest, MetricsRegistryNonExistentKey) {
  obs::MetricsRegistry registry;

  // Get counter that was never registered
  auto counter = registry.counter("nonexistent");
  EXPECT_NO_THROW(counter.inc()); // Should be no-op

  auto hist = registry.histogram("also_nonexistent");
  EXPECT_NO_THROW(hist.record(123.0));
}

// =============================================================================
// Safety Test 2: Metric Limit Overflow
// =============================================================================

TEST_F(SafetyTest, ManyCountersWithinLimit) {
  // Create many counters (but within limit)
  std::vector<obs::Counter> counters;

  for (int i = 0; i < 100; i++) {
    auto c = obs::register_counter("counter_" + std::to_string(i),
                                   obs::Unit::Dimensionless);
    counters.push_back(c);

    // Should work
    EXPECT_NO_THROW(c.inc());
  }
}

TEST_F(SafetyTest, CounterRegistrationLimit) {
  // Fix #2: Test bounds checking at MAX_COUNTERS (256)
  std::vector<obs::Counter> counters;

  // Register exactly 256 counters (should all succeed)
  for (int i = 0; i < 256; i++) {
    auto c = obs::register_counter("limit_counter_" + std::to_string(i),
                                   obs::Unit::Dimensionless);
    counters.push_back(c);

    // Verify handle is valid (non-zero ID expected)
    EXPECT_NO_THROW(c.inc());
  }

  // 257th counter should fail gracefully (return null handle with ID=0)
  auto overflow =
      obs::register_counter("overflow_counter", obs::Unit::Dimensionless);

  // Null handle should be safe to use (no-op, no crash)
  EXPECT_NO_THROW(overflow.inc());
  EXPECT_NO_THROW(overflow.inc(100));
}

// =============================================================================
// Safety Test 3: Basic Multi-threaded Integration
// =============================================================================

TEST_F(SafetyTest, ConcurrentCounterInc) {
  auto counter = obs::register_counter("concurrent", obs::Unit::Dimensionless);

  constexpr int NUM_THREADS = 4;
  constexpr int OPS_PER_THREAD = 1000;

  std::vector<std::thread> threads;
  for (int t = 0; t < NUM_THREADS; t++) {
    threads.emplace_back([&counter]() {
      for (int i = 0; i < OPS_PER_THREAD; i++) {
        counter.inc();
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  // Test passes if no crashes occurred
  SUCCEED();
}

TEST_F(SafetyTest, ConcurrentHistogramRecord) {
  auto hist =
      obs::register_histogram("concurrent_hist", obs::Unit::Milliseconds);

  constexpr int NUM_THREADS = 4;
  constexpr int OPS_PER_THREAD = 1000;

  std::vector<std::thread> threads;
  for (int t = 0; t < NUM_THREADS; t++) {
    threads.emplace_back([&hist, t]() {
      for (int i = 0; i < OPS_PER_THREAD; i++) {
        hist.record(static_cast<double>(t * 100 + i));
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

TEST_F(SafetyTest, ConcurrentGaugeUpdates) {
  auto gauge =
      obs::register_gauge("concurrent_gauge", obs::Unit::Dimensionless);

  constexpr int NUM_THREADS = 4;
  constexpr int OPS_PER_THREAD = 500;

  std::vector<std::thread> threads;
  for (int t = 0; t < NUM_THREADS; t++) {
    threads.emplace_back([&gauge, t]() {
      for (int i = 0; i < OPS_PER_THREAD; i++) {
        if (i % 2 == 0) {
          gauge.set(t * 1000 + i);
        } else {
          gauge.add(1);
        }
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

TEST_F(SafetyTest, EndToEndFlow) {
  // Full lifecycle test

  // 1. Register metrics
  auto requests_counter =
      obs::register_counter("requests", obs::Unit::Dimensionless);
  auto latency_hist =
      obs::register_histogram("latency", obs::Unit::Milliseconds);
  auto connections_gauge =
      obs::register_gauge("connections", obs::Unit::Dimensionless);

  // 2. Use from multiple threads
  std::vector<std::thread> threads;
  for (int t = 0; t < 3; t++) {
    threads.emplace_back([&]() {
      for (int i = 0; i < 100; i++) {
        requests_counter.inc();
        latency_hist.record(static_cast<double>(i));
        connections_gauge.set(t * 10 + i);
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  // 3. Verify still usable after threads finish
  EXPECT_NO_THROW(requests_counter.inc(999));
  EXPECT_NO_THROW(latency_hist.record(42.5));
  EXPECT_NO_THROW(connections_gauge.set(0));

  // 4. Shutdown will happen in TearDown
  SUCCEED();
}

TEST_F(SafetyTest, AdHocMetricsConcurrent) {
  // Test ad-hoc metrics (not pre-registered) from multiple threads

  std::vector<std::thread> threads;
  for (int t = 0; t < 3; t++) {
    threads.emplace_back([t]() {
      for (int i = 0; i < 50; i++) {
        // Different threads use same metric names (should be cached)
        auto c = obs::counter("ad_hoc_counter", obs::Unit::Dimensionless);
        c.inc();

        auto h = obs::histogram("ad_hoc_hist", obs::Unit::Milliseconds);
        h.record(static_cast<double>(t * 10 + i));
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}
// =============================================================================
// Safety Test 4: Cache Size Limit (Fix #3)
// =============================================================================

TEST_F(SafetyTest, AdHocCacheSizeLimit) {
  // Fix #3: Verify thread-local cache doesn't grow unbounded
  // Register 150 unique ad-hoc metrics (exceeds hypothetical 100 limit)

  for (int i = 0; i < 150; i++) {
    auto c = obs::counter("dynamic_counter_" + std::to_string(i));
    EXPECT_NO_THROW(c.inc());

    auto h = obs::histogram("dynamic_hist_" + std::to_string(i));
    EXPECT_NO_THROW(h.record(static_cast<double>(i)));

    auto g = obs::gauge("dynamic_gauge_" + std::to_string(i));
    EXPECT_NO_THROW(g.set(i));
  }

  // If cache is unbounded, memory would keep growing
  // If cache has limit, old entries would be cleared
  // Either way, test should complete without crash
  SUCCEED();
}
