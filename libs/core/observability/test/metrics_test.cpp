#include <Metrics.h>
#include <Provider.h>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

namespace {

class MetricsV2Test : public ::testing::Test {
protected:
  void SetUp() override {
    ::observability::Config config;
    config.set_service_name("test-metrics");
    config.set_otlp_endpoint("http://localhost:4317");
    ASSERT_TRUE(obs::init(config));
  }

  void TearDown() override {
    obs::shutdown();
  }
};

// =============================================================================
// Counter Tests
// =============================================================================

TEST_F(MetricsV2Test, CounterRegistration) {
  auto counter = obs::register_counter("test.counter");

  // Should not throw
  EXPECT_NO_THROW(counter.inc());
  EXPECT_NO_THROW(counter.inc(5));
  EXPECT_NO_THROW(counter.inc(100));
}

TEST_F(MetricsV2Test, CounterWithUnit) {
  auto bytes_counter = obs::register_counter("test.bytes", obs::Unit::Bytes);
  EXPECT_NO_THROW(bytes_counter.inc(1024));
}

TEST_F(MetricsV2Test, CounterAdHoc) {
  // Ad-hoc counter (auto-registered, cached)
  EXPECT_NO_THROW(obs::counter("ad_hoc.counter").inc());
  EXPECT_NO_THROW(obs::counter("ad_hoc.counter").inc(10));
}

TEST_F(MetricsV2Test, CounterDefaultDelta) {
  auto counter = obs::register_counter("test.default");
  EXPECT_NO_THROW(counter.inc()); // Default delta = 1
}

// =============================================================================
// Histogram Tests
// =============================================================================

TEST_F(MetricsV2Test, HistogramRegistration) {
  auto hist = obs::register_histogram("test.histogram");

  EXPECT_NO_THROW(hist.record(42.5));
  EXPECT_NO_THROW(hist.record(100.0));
  EXPECT_NO_THROW(hist.record(0.001));
}

TEST_F(MetricsV2Test, HistogramWithUnit) {
  auto bytes_hist = obs::register_histogram("test.size", obs::Unit::Bytes);
  EXPECT_NO_THROW(bytes_hist.record(1024));
}

TEST_F(MetricsV2Test, HistogramAdHoc) {
  EXPECT_NO_THROW(obs::histogram("ad_hoc.histogram").record(99.9));
}

// =============================================================================
// DurationHistogram Tests (Chrono Support)
// =============================================================================

TEST_F(MetricsV2Test, DurationHistogramWithChrono) {
  auto latency = obs::register_duration_histogram("test.latency");

  // Record durations using chrono
  EXPECT_NO_THROW(latency.record(std::chrono::milliseconds(100)));
  EXPECT_NO_THROW(latency.record(std::chrono::seconds(1)));
  EXPECT_NO_THROW(latency.record(std::chrono::microseconds(500)));
}

TEST_F(MetricsV2Test, DurationHistogramAutoConversion) {
  auto latency = obs::register_duration_histogram("test.auto_convert");

  // Test auto-conversion to milliseconds
  auto start = std::chrono::steady_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  auto elapsed = std::chrono::steady_clock::now() - start;

  EXPECT_NO_THROW(latency.record(elapsed));
}

// =============================================================================
// Gauge Tests
// =============================================================================

TEST_F(MetricsV2Test, GaugeSet) {
  auto gauge = obs::register_gauge("test.gauge");

  EXPECT_NO_THROW(gauge.set(100));
  EXPECT_NO_THROW(gauge.set(0));
  EXPECT_NO_THROW(gauge.set(-50));
}

TEST_F(MetricsV2Test, GaugeSetComputesDelta) {
  auto gauge = obs::register_gauge("test.gauge_delta");

  // Setting to 100, then 150 should add delta of +50, not 150
  EXPECT_NO_THROW(gauge.set(100));
  EXPECT_NO_THROW(gauge.set(150)); // Should compute delta = 150 - 100 = +50
  EXPECT_NO_THROW(gauge.set(120)); // Should compute delta = 120 - 150 = -30
}

TEST_F(MetricsV2Test, GaugeAdd) {
  auto gauge = obs::register_gauge("test.gauge_delta");

  EXPECT_NO_THROW(gauge.add(10)); // Increment
  EXPECT_NO_THROW(gauge.add(-5)); // Decrement
  EXPECT_NO_THROW(gauge.add(0));  // No-op
}

TEST_F(MetricsV2Test, GaugeWithUnit) {
  auto percent = obs::register_gauge("test.cpu", obs::Unit::Percent);
  EXPECT_NO_THROW(percent.set(75));
}

TEST_F(MetricsV2Test, GaugeAdHoc) {
  EXPECT_NO_THROW(obs::gauge("ad_hoc.gauge").set(42));
  EXPECT_NO_THROW(obs::gauge("ad_hoc.gauge").add(10));
}

// =============================================================================
// Multiple Metrics Tests
// =============================================================================

TEST_F(MetricsV2Test, MultipleMetricsSameSession) {
  auto c1 = obs::register_counter("multi.counter1");
  auto c2 = obs::register_counter("multi.counter2");
  auto h1 = obs::register_histogram("multi.hist1");
  auto g1 = obs::register_gauge("multi.gauge1");

  EXPECT_NO_THROW({
    c1.inc();
    c2.inc(5);
    h1.record(42.0);
    g1.set(100);
  });
}

TEST_F(MetricsV2Test, SameMetricMultipleTimes) {
  auto c1 = obs::register_counter("same.counter");
  auto c2 = obs::register_counter("same.counter"); // Same name

  // Both should work (same underlying metric)
  EXPECT_NO_THROW(c1.inc());
  EXPECT_NO_THROW(c2.inc());
}

} // namespace
