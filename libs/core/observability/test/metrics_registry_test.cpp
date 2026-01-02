#include <MetricsRegistry.h>
#include <Provider.h>
#include <gtest/gtest.h>

class MetricsRegistryTest : public ::testing::Test {
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
// Fluent Registration
// =============================================================================

TEST_F(MetricsRegistryTest, FluentCounterRegistration) {
  obs::MetricsRegistry metrics;

  // Fluent API should return *this for chaining
  auto &result = metrics.counter("requests", "http.requests.total");
  EXPECT_EQ(&result, &metrics); // Returns reference to same object
}

TEST_F(MetricsRegistryTest, ChainedRegistration) {
  obs::MetricsRegistry metrics;

  // Should be able to chain multiple registrations
  EXPECT_NO_THROW(metrics.counter("requests", "http.requests.total")
                      .counter("errors", "http.errors.total")
                      .histogram("latency", "http.latency_ms")
                      .gauge("connections", "http.active_connections"));
}

TEST_F(MetricsRegistryTest, LookupByKey) {
  obs::MetricsRegistry metrics;
  metrics.counter("requests", "http.requests.total");

  auto counter = metrics.counter("requests");
  EXPECT_NO_THROW(counter.inc());
  EXPECT_NO_THROW(counter.inc(5));
}

TEST_F(MetricsRegistryTest, HistogramLookup) {
  obs::MetricsRegistry metrics;
  metrics.histogram("latency", "http.latency_ms");

  auto hist = metrics.histogram("latency");
  EXPECT_NO_THROW(hist.record(42.5));
}

TEST_F(MetricsRegistryTest, GaugeLookup) {
  obs::MetricsRegistry metrics;
  metrics.gauge("memory", "process.memory_bytes", obs::Unit::Bytes);

  auto gauge = metrics.gauge("memory");
  EXPECT_NO_THROW(gauge.set(1024));
  EXPECT_NO_THROW(gauge.add(512));
}

TEST_F(MetricsRegistryTest, DurationHistogramLookup) {
  obs::MetricsRegistry metrics;
  metrics.duration_histogram("db_query", "db.query.latency_ms");

  auto hist = metrics.duration_histogram("db_query");
  EXPECT_NO_THROW(hist.record(std::chrono::milliseconds(100)));
}

// =============================================================================
// Safety (Non-existent Keys)
// =============================================================================

TEST_F(MetricsRegistryTest, NonExistentCounterSafe) {
  obs::MetricsRegistry metrics;

  // Should return null counter that's safe to use
  auto counter = metrics.counter("nonexistent");
  EXPECT_NO_THROW(counter.inc()); // No-op, but doesn't crash
}

TEST_F(MetricsRegistryTest, RealWorldUsage) {
  obs::MetricsRegistry metrics;

  // Setup all metrics in constructor
  metrics.counter("requests", "http.requests", obs::Unit::Dimensionless)
      .counter("errors", "http.errors", obs::Unit::Dimensionless)
      .histogram("latency", "http.latency", obs::Unit::Milliseconds)
      .gauge("connections", "http.connections", obs::Unit::Dimensionless);

  // Use them throughout application
  metrics.counter("requests").inc();
  metrics.histogram("latency").record(12.5);
  metrics.gauge("connections").set(42);

  // All should work
  auto counter = metrics.counter("requests");
  EXPECT_NO_THROW(counter.inc(10));
}
