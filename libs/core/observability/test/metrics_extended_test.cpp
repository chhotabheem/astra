#include <Metrics.h>
#include <Provider.h>
#include <cmath>
#include <gtest/gtest.h>
#include <limits>
#include <thread>
#include <vector>

class MetricsExtendedTest : public ::testing::Test {
protected:
  void SetUp() override {
    ::observability::Config config;
    config.set_service_name("metrics-test");
    obs::init(config);
  }

  void TearDown() override {
    obs::shutdown();
  }
};

// ============================================================================
// Counter Tests
// ============================================================================

TEST_F(MetricsExtendedTest, CounterOverflow) {
  auto counter = obs::counter("overflow.test");

  // Inc close to max
  for (int i = 0; i < 100; ++i) {
    counter.inc(std::numeric_limits<int64_t>::max() / 1000);
  }

  // Should not crash
  SUCCEED();
}

TEST_F(MetricsExtendedTest, CounterConcurrent100Threads) {
  auto counter = obs::counter("concurrent.counter");
  std::vector<std::thread> threads;

  for (int i = 0; i < 100; ++i) {
    threads.emplace_back([&counter]() {
      for (int j = 0; j < 1000; ++j) {
        counter.inc();
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

TEST_F(MetricsExtendedTest, CounterWithAllAttributeTypes) {
  auto counter = obs::counter("attr.counter");

  EXPECT_NO_THROW(counter.inc(1, {{"str", "val"}}));
  EXPECT_NO_THROW(counter.inc(1, {{"int", std::to_string(42)}}));
  EXPECT_NO_THROW(counter.inc(1, {{"bool", "true"}}));
}

TEST_F(MetricsExtendedTest, CounterWithEmptyAttributes) {
  auto counter = obs::counter("empty.attr");
  EXPECT_NO_THROW(counter.inc(1, {}));
}

TEST_F(MetricsExtendedTest, CounterWithDuplicateAttributeKeys) {
  auto counter = obs::counter("dup.attr");
  EXPECT_NO_THROW(counter.inc(1, {{"key", "val1"}, {"key", "val2"}}));
}

TEST_F(MetricsExtendedTest, CounterWithLongAttributeValue) {
  auto counter = obs::counter("long.attr");
  std::string long_val(10000, 'x');
  EXPECT_NO_THROW(counter.inc(1, {{"key", long_val}}));
}

TEST_F(MetricsExtendedTest, CounterAfterShutdown) {
  auto counter = obs::counter("shutdown.counter");
  obs::shutdown();

  // Should not crash
  EXPECT_NO_THROW(counter.inc());
}

TEST_F(MetricsExtendedTest, CounterMaxBoundary) {
  // Register 256 counters (MAX_COUNTERS)
  for (int i = 0; i < 256; ++i) {
    auto counter = obs::counter("counter." + std::to_string(i));
    counter.inc();
  }

  // 257th should return sentinel
  auto overflow_counter = obs::counter("counter.257");
  EXPECT_NO_THROW(overflow_counter.inc());
}

TEST_F(MetricsExtendedTest, CounterZeroIncrement) {
  auto counter = obs::counter("zero.inc");
  EXPECT_NO_THROW(counter.inc(0));
}

TEST_F(MetricsExtendedTest, CounterLargeIncrement) {
  auto counter = obs::counter("large.inc");
  EXPECT_NO_THROW(counter.inc(1000000000));
}

// ============================================================================
// Histogram Tests
// ============================================================================

TEST_F(MetricsExtendedTest, HistogramNegativeValue) {
  auto hist = obs::histogram("negative.hist");
  EXPECT_NO_THROW(hist.record(-100.0));
}

TEST_F(MetricsExtendedTest, HistogramZeroValue) {
  auto hist = obs::histogram("zero.hist");
  EXPECT_NO_THROW(hist.record(0.0));
}

TEST_F(MetricsExtendedTest, HistogramNaN) {
  auto hist = obs::histogram("nan.hist");
  EXPECT_NO_THROW(hist.record(std::numeric_limits<double>::quiet_NaN()));
}

TEST_F(MetricsExtendedTest, HistogramInfinity) {
  auto hist = obs::histogram("inf.hist");
  EXPECT_NO_THROW(hist.record(std::numeric_limits<double>::infinity()));
  EXPECT_NO_THROW(hist.record(-std::numeric_limits<double>::infinity()));
}

TEST_F(MetricsExtendedTest, HistogramVerySmallValue) {
  auto hist = obs::histogram("small.hist");
  EXPECT_NO_THROW(hist.record(std::numeric_limits<double>::min()));
  EXPECT_NO_THROW(hist.record(1e-308));
}

TEST_F(MetricsExtendedTest, HistogramVeryLargeValue) {
  auto hist = obs::histogram("large.hist");
  EXPECT_NO_THROW(hist.record(std::numeric_limits<double>::max()));
  EXPECT_NO_THROW(hist.record(1e308));
}

TEST_F(MetricsExtendedTest, HistogramWithAttributes) {
  auto hist = obs::histogram("attr.hist");
  EXPECT_NO_THROW(hist.record(42.0, {{"method", "GET"}, {"status", "200"}}));
}

TEST_F(MetricsExtendedTest, HistogramConcurrent) {
  auto hist = obs::histogram("concurrent.hist");
  std::vector<std::thread> threads;

  for (int i = 0; i < 50; ++i) {
    threads.emplace_back([&hist, i]() {
      for (int j = 0; j < 100; ++j) {
        hist.record(static_cast<double>(i * j));
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

TEST_F(MetricsExtendedTest, HistogramMaxBoundary) {
  for (int i = 0; i < 256; ++i) {
    auto hist = obs::histogram("hist." + std::to_string(i));
    hist.record(1.0);
  }

  auto overflow = obs::histogram("hist.257");
  EXPECT_NO_THROW(overflow.record(1.0));
}

TEST_F(MetricsExtendedTest, HistogramAfterShutdown) {
  auto hist = obs::histogram("shutdown.hist");
  obs::shutdown();
  EXPECT_NO_THROW(hist.record(42.0));
}

// ============================================================================
// Gauge Tests
// ============================================================================

TEST_F(MetricsExtendedTest, GaugeNegativeSet) {
  auto gauge = obs::gauge("negative.gauge");
  EXPECT_NO_THROW(gauge.set(-100));
}

TEST_F(MetricsExtendedTest, GaugeZeroSet) {
  auto gauge = obs::gauge("zero.gauge");
  EXPECT_NO_THROW(gauge.set(0));
}

TEST_F(MetricsExtendedTest, GaugeLargeValue) {
  auto gauge = obs::gauge("large.gauge");
  EXPECT_NO_THROW(gauge.set(std::numeric_limits<int64_t>::max()));
  EXPECT_NO_THROW(gauge.set(std::numeric_limits<int64_t>::min()));
}

TEST_F(MetricsExtendedTest, GaugeAddPositive) {
  auto gauge = obs::gauge("add.positive");
  gauge.set(0);
  EXPECT_NO_THROW(gauge.add(100));
}

TEST_F(MetricsExtendedTest, GaugeAddNegative) {
  auto gauge = obs::gauge("add.negative");
  gauge.set(100);
  EXPECT_NO_THROW(gauge.add(-50));
}

TEST_F(MetricsExtendedTest, GaugeAddZero) {
  auto gauge = obs::gauge("add.zero");
  gauge.set(42);
  EXPECT_NO_THROW(gauge.add(0));
}

TEST_F(MetricsExtendedTest, GaugeConcurrentAdditions) {
  auto gauge = obs::gauge("concurrent.gauge");
  gauge.set(0);

  std::vector<std::thread> threads;
  for (int i = 0; i < 50; ++i) {
    threads.emplace_back([&gauge]() {
      for (int j = 0; j < 100; ++j) {
        gauge.add(1);
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

TEST_F(MetricsExtendedTest, GaugeSetVsAddInterleaving) {
  auto gauge = obs::gauge("interleave.gauge");

  std::thread t1([&gauge]() {
    for (int i = 0; i < 100; ++i) {
      gauge.set(i);
    }
  });

  std::thread t2([&gauge]() {
    for (int i = 0; i < 100; ++i) {
      gauge.add(1);
    }
  });

  t1.join();
  t2.join();

  SUCCEED();
}

TEST_F(MetricsExtendedTest, GaugeMaxBoundary) {
  for (int i = 0; i < 256; ++i) {
    auto gauge = obs::gauge("gauge." + std::to_string(i));
    gauge.set(i);
  }

  auto overflow = obs::gauge("gauge.257");
  EXPECT_NO_THROW(overflow.set(1));
}

TEST_F(MetricsExtendedTest, GaugeAddOverflow) {
  auto gauge = obs::gauge("overflow.gauge");
  gauge.set(std::numeric_limits<int64_t>::max() - 10);
  EXPECT_NO_THROW(gauge.add(20)); // Should overflow
}

// ============================================================================
// DurationHistogram Tests
// ============================================================================

TEST_F(MetricsExtendedTest, DurationHistogramNanosecondPrecision) {
  auto hist = obs::register_duration_histogram("nano.duration");
  auto duration = std::chrono::nanoseconds(1);
  EXPECT_NO_THROW(hist.record(duration));
}

TEST_F(MetricsExtendedTest, DurationHistogramZeroDuration) {
  auto hist = obs::register_duration_histogram("zero.duration");
  auto duration = std::chrono::milliseconds(0);
  EXPECT_NO_THROW(hist.record(duration));
}

TEST_F(MetricsExtendedTest, DurationHistogramVeryLongDuration) {
  auto hist = obs::register_duration_histogram("long.duration");
  auto duration = std::chrono::hours(24 * 365); // 1 year
  EXPECT_NO_THROW(hist.record(duration));
}

TEST_F(MetricsExtendedTest, DurationHistogramConcurrent) {
  auto hist = obs::register_duration_histogram("concurrent.duration");
  std::vector<std::thread> threads;

  for (int i = 0; i < 50; ++i) {
    threads.emplace_back([&hist, i]() {
      for (int j = 0; j < 100; ++j) {
        hist.record(std::chrono::milliseconds(i + j));
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

TEST_F(MetricsExtendedTest, DurationHistogramDifferentChronoTypes) {
  auto hist = obs::register_duration_histogram("chrono.types");

  EXPECT_NO_THROW(hist.record(std::chrono::nanoseconds(100)));
  EXPECT_NO_THROW(hist.record(std::chrono::microseconds(100)));
  EXPECT_NO_THROW(hist.record(std::chrono::milliseconds(100)));
  EXPECT_NO_THROW(hist.record(std::chrono::seconds(1)));
  EXPECT_NO_THROW(hist.record(std::chrono::minutes(1)));
  EXPECT_NO_THROW(hist.record(std::chrono::hours(1)));
}

TEST_F(MetricsExtendedTest, DurationHistogramWithAttributes) {
  auto hist = obs::register_duration_histogram("attr.duration");
  auto duration = std::chrono::milliseconds(100);
  EXPECT_NO_THROW(hist.record(duration, {{"operation", "query"}}));
}

// ============================================================================
// Mixed Metric Type Tests
// ============================================================================

TEST_F(MetricsExtendedTest, AllMetricTypesConcurrent) {
  auto counter = obs::counter("mixed.counter");
  auto hist = obs::histogram("mixed.hist");
  auto gauge = obs::gauge("mixed.gauge");
  auto duration = obs::register_duration_histogram("mixed.duration");

  std::vector<std::thread> threads;

  threads.emplace_back([&counter]() {
    for (int i = 0; i < 1000; ++i) {
      counter.inc();
    }
  });

  threads.emplace_back([&hist]() {
    for (int i = 0; i < 1000; ++i) {
      hist.record(static_cast<double>(i));
    }
  });

  threads.emplace_back([&gauge]() {
    for (int i = 0; i < 1000; ++i) {
      gauge.add(1);
    }
  });

  threads.emplace_back([&duration]() {
    for (int i = 0; i < 1000; ++i) {
      duration.record(std::chrono::milliseconds(i));
    }
  });

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

TEST_F(MetricsExtendedTest, SameNameDifferentTypes) {
  // Different metric types can have same name (different namespaces)
  auto counter = obs::counter("same.name");
  auto hist = obs::histogram("same.name");
  auto gauge = obs::gauge("same.name");

  EXPECT_NO_THROW(counter.inc());
  EXPECT_NO_THROW(hist.record(1.0));
  EXPECT_NO_THROW(gauge.set(1));
}

TEST_F(MetricsExtendedTest, MetricNameWithSpecialCharacters) {
  auto counter = obs::counter("metric-name.with_special!@#$%^&*()chars");
  EXPECT_NO_THROW(counter.inc());
}

TEST_F(MetricsExtendedTest, MetricNameVeryLong) {
  std::string long_name(10000, 'x');
  auto counter = obs::counter(long_name);
  EXPECT_NO_THROW(counter.inc());
}

TEST_F(MetricsExtendedTest, MetricNameEmpty) {
  auto counter = obs::counter("");
  EXPECT_NO_THROW(counter.inc());
}
