// Test file for attributes
#include <Metrics.h>
#include <Provider.h>
#include <gtest/gtest.h>

class AttributesTest : public ::testing::Test {
protected:
  void SetUp() override {
    ::observability::Config config;
    config.set_service_name("test-service");
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
// Counter with Attributes
// =============================================================================

TEST_F(AttributesTest, CounterWithSingleAttribute) {
  auto counter =
      obs::register_counter("test.counter", obs::Unit::Dimensionless);

  EXPECT_NO_THROW(counter.inc(1, {{"endpoint", "/api"}}));
}

TEST_F(AttributesTest, CounterWithMultipleAttributes) {
  auto counter =
      obs::register_counter("test.counter_multi", obs::Unit::Dimensionless);

  EXPECT_NO_THROW(counter.inc(
      1, {{"endpoint", "/api"}, {"method", "POST"}, {"status", "200"}}));
}

TEST_F(AttributesTest, CounterWithoutAttributes) {
  auto counter =
      obs::register_counter("test.counter_no_attrs", obs::Unit::Dimensionless);

  // Should still work without attributes
  EXPECT_NO_THROW(counter.inc(1));
}

// =============================================================================
// Histogram with Attributes
// =============================================================================

TEST_F(AttributesTest, HistogramWithAttributes) {
  auto hist = obs::register_histogram("test.latency", obs::Unit::Milliseconds);

  EXPECT_NO_THROW(hist.record(42.5, {{"endpoint", "/users"}}));
}

// =============================================================================
// Gauge with Attributes
// =============================================================================

TEST_F(AttributesTest, GaugeSetWithAttributes) {
  auto gauge = obs::register_gauge("test.memory", obs::Unit::Bytes);

  EXPECT_NO_THROW(gauge.set(1024, {{"region", "us-west"}}));
}

TEST_F(AttributesTest, GaugeAddWithAttributes) {
  auto gauge = obs::register_gauge("test.queue_size", obs::Unit::Dimensionless);

  EXPECT_NO_THROW(gauge.add(10, {{"queue", "high-priority"}}));
  EXPECT_NO_THROW(gauge.add(-5, {{"queue", "high-priority"}}));
}

// =============================================================================
// DurationHistogram with Attributes (Fix #1)
// =============================================================================

TEST_F(AttributesTest, DurationHistogramWithAttributes) {
  auto hist = obs::register_duration_histogram("test.duration_latency");

  // Should support attributes like regular Histogram
  EXPECT_NO_THROW(
      hist.record(std::chrono::milliseconds(100), {{"endpoint", "/api"}}));
  EXPECT_NO_THROW(
      hist.record(std::chrono::seconds(2), {{"endpoint", "/users"}}));
}

TEST_F(AttributesTest, DurationHistogramWithMultipleAttributes) {
  auto hist = obs::register_duration_histogram("test.duration_multi");

  EXPECT_NO_THROW(hist.record(
      std::chrono::milliseconds(250),
      {{"endpoint", "/api"}, {"method", "POST"}, {"cache", "miss"}}));
}
