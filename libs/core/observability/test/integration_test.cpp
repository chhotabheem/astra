#include <Log.h>
#include <Metrics.h>
#include <MetricsRegistry.h>
#include <Provider.h>
#include <Span.h>
#include <Tracer.h>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

class IntegrationTest : public ::testing::Test {
protected:
  std::shared_ptr<obs::Tracer> tracer;

  void SetUp() override {
    ::observability::Config config;
    config.set_service_name("integration-test");
    config.set_service_version("1.0.0");
    obs::init(config);
    tracer = obs::Provider::instance().get_tracer("integration-test");
  }

  void TearDown() override {
    tracer.reset();
    obs::shutdown();
  }
};

TEST_F(IntegrationTest, FullObservabilityStack) {
  // Simulate a complete request with metrics, tracing, and logging

  auto request_counter = obs::register_counter("test.requests");
  auto latency_hist = obs::register_duration_histogram("test.latency");

  {
    obs::ScopedLogAttributes scoped(
        {{"request.id", "req-12345"}, {"client.ip", "192.168.1.1"}});

    auto span = tracer->start_span("handle_request");
    span->kind(obs::SpanKind::Server);
    span->attr("http.method", "GET");
    span->attr("http.route", "/api/test");

    auto start = std::chrono::steady_clock::now();

    obs::info("Request started");
    request_counter.inc();

    // Simulate work
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    span->add_event("processing_started");

    {
      auto db_span = tracer->start_span("database_query", span->context());
      db_span->kind(obs::SpanKind::Client);
      db_span->attr("db.system", "postgresql");

      obs::debug("Executing database query");

      // Simulate DB work
      std::this_thread::sleep_for(std::chrono::milliseconds(5));

      db_span->set_status(obs::StatusCode::Ok);
      db_span->end();
    }

    auto duration = std::chrono::steady_clock::now() - start;
    latency_hist.record(duration);

    span->set_status(obs::StatusCode::Ok);
    span->end();
    obs::info(
        "Request completed",
        {{"duration_ms",
          std::to_string(
              std::chrono::duration_cast<std::chrono::milliseconds>(duration)
                  .count())}});
  }

  // All spans auto-ended, metrics recorded, logs emitted
}

TEST_F(IntegrationTest, ErrorHandling) {
  auto error_counter = obs::register_counter("test.errors");

  {
    auto span = tracer->start_span("failing_operation");

    try {
      obs::warn("About to fail");
      throw std::runtime_error("Simulated error");
    } catch (const std::exception &e) {
      error_counter.inc();
      span->set_status(obs::StatusCode::Error, e.what());
      obs::error("Operation failed", {{"error.type", "runtime_error"},
                                      {"error.message", e.what()}});
    }
    span->end();
  }
}

TEST_F(IntegrationTest, MetricsRegistryWithSpansAndLogs) {
  obs::MetricsRegistry metrics;
  metrics.counter("requests", "test.requests.total")
      .counter("errors", "test.errors.total")
      .duration_histogram("latency", "test.latency")
      .gauge("active", "test.active_requests");

  {
    auto span = tracer->start_span("operation");
    obs::ScopedLogAttributes scoped({{"operation", "test"}});

    metrics.counter("requests").inc();
    metrics.gauge("active").add(1);

    obs::info("Operation started");

    auto start = std::chrono::steady_clock::now();

    // Simulate work
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    auto duration = std::chrono::steady_clock::now() - start;
    metrics.duration_histogram("latency").record(duration);

    metrics.gauge("active").add(-1);

    span->set_status(obs::StatusCode::Ok);
    span->end();
    obs::info("Operation completed");
  }
}

TEST_F(IntegrationTest, NestedSpansWithMetricsAndLogs) {
  auto counter = obs::register_counter("nested.operations");

  {
    auto parent_span = tracer->start_span("parent");
    parent_span->attr("level", "parent");

    obs::info("Parent operation started");
    counter.inc();

    {
      auto child_span = tracer->start_span("child", parent_span->context());
      child_span->attr("level", "child");

      obs::debug("Child operation started");

      {
        auto grandchild_span =
            tracer->start_span("grandchild", child_span->context());
        grandchild_span->attr("level", "grandchild");

        obs::trace("Grandchild operation");

        grandchild_span->set_status(obs::StatusCode::Ok);
        grandchild_span->end();
      }

      child_span->set_status(obs::StatusCode::Ok);
      child_span->end();
    }

    parent_span->set_status(obs::StatusCode::Ok);
    parent_span->end();
    obs::info("Parent operation completed");
  }
}

TEST_F(IntegrationTest, MultipleAttributeTypes) {
  auto span = tracer->start_span("typed_attributes");

  span->attr("string_attr", "value");
  span->attr("int_attr", static_cast<int64_t>(42));
  span->attr("double_attr", 3.14);
  span->attr("bool_attr", true);

  obs::info("Multiple attribute types",
            {{"attr1", "string"}, {"attr2", "value2"}});

  span->set_status(obs::StatusCode::Ok);
  span->end();
}
