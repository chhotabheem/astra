#include <Log.h>
#include <Metrics.h>
#include <MetricsRegistry.h>
#include <Provider.h>
#include <Span.h>
#include <Tracer.h>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

class IntegrationExtendedTest : public ::testing::Test {
protected:
  std::shared_ptr<obs::Tracer> tracer;

  void SetUp() override {
    ::observability::Config config;
    config.set_service_name("integration-test");
    config.set_environment("test");
    obs::init(config);
    tracer = obs::Provider::instance().get_tracer("integration-test");
  }

  void TearDown() override {
    tracer.reset();
    obs::shutdown();
  }
};

// HTTP request full flow
TEST_F(IntegrationExtendedTest, HTTPRequestFullFlow) {
  obs::MetricsRegistry metrics;
  metrics.counter("requests", "http.requests")
      .duration_histogram("latency", "http.latency");

  auto span = tracer->start_span("http.request");
  span->kind(obs::SpanKind::Server)
      .attr("method", "GET")
      .attr("path", "/api/users");

  obs::info("Request started", {{"method", "GET"}});

  auto start = std::chrono::steady_clock::now();

  // Simulate processing
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  metrics.counter("requests").inc();
  metrics.duration_histogram("latency").record(
      std::chrono::steady_clock::now() - start);

  span->set_status(obs::StatusCode::Ok);
  span->end();
  obs::info("Request completed", {{"status", "200"}});

  SUCCEED();
}

// Database query flow
TEST_F(IntegrationExtendedTest, DatabaseQueryFlow) {
  auto parent = tracer->start_span("api.call");
  parent->kind(obs::SpanKind::Server);

  {
    auto db_span = tracer->start_span("db.query", parent->context());
    db_span->kind(obs::SpanKind::Client)
        .attr("db.system", "postgresql")
        .attr("db.operation", "SELECT");

    obs::debug("Executing query", {{"table", "users"}});

    // Simulate query
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    db_span->set_status(obs::StatusCode::Ok);
    db_span->end();
  }

  parent->set_status(obs::StatusCode::Ok);
  parent->end();

  SUCCEED();
}

// Error handling flow
TEST_F(IntegrationExtendedTest, ErrorHandlingFlow) {
  obs::MetricsRegistry metrics;
  metrics.counter("errors", "app.errors");

  auto span = tracer->start_span("operation.with.error");

  try {
    throw std::runtime_error("Simulated error");
  } catch (const std::exception &e) {
    span->set_status(obs::StatusCode::Error, e.what());
    obs::error("Operation failed", {{"error", e.what()}});
    metrics.counter("errors").inc();
  }
  span->end();

  SUCCEED();
}

// Retry logic with observability
TEST_F(IntegrationExtendedTest, RetryLogicWithObservability) {
  obs::MetricsRegistry metrics;
  metrics.counter("retries", "operation.retries");

  auto span = tracer->start_span("operation.with.retries");

  for (int attempt = 1; attempt <= 3; ++attempt) {
    span->add_event("retry.attempt", {{"attempt", std::to_string(attempt)}});

    obs::warn("Retry attempt", {{"attempt", std::to_string(attempt)}});

    if (attempt < 3) {
      metrics.counter("retries").inc();
    }
  }

  span->set_status(obs::StatusCode::Ok);
  span->end();

  SUCCEED();
}

// Fan-out/fan-in pattern
TEST_F(IntegrationExtendedTest, FanOutFanInPattern) {
  auto parent = tracer->start_span("fan.out.operation");
  auto parent_ctx = parent->context();
  auto local_tracer = tracer;

  std::vector<std::thread> workers;

  for (int i = 0; i < 5; ++i) {
    workers.emplace_back([local_tracer, parent_ctx, i]() {
      auto worker_span =
          local_tracer->start_span("worker." + std::to_string(i), parent_ctx);
      worker_span->attr("worker_id", static_cast<int64_t>(i));

      obs::debug("Worker processing", {{"id", std::to_string(i)}});

      std::this_thread::sleep_for(std::chrono::milliseconds(5));

      worker_span->set_status(obs::StatusCode::Ok);
      worker_span->end();
    });
  }

  for (auto &worker : workers) {
    worker.join();
  }

  parent->set_status(obs::StatusCode::Ok);
  parent->end();

  SUCCEED();
}

// Batch processing
TEST_F(IntegrationExtendedTest, BatchProcessing) {
  obs::MetricsRegistry metrics;
  metrics.counter("processed", "batch.items.processed")
      .histogram("batchsize", "batch.size");

  auto batch_span = tracer->start_span("batch.process");

  constexpr int batch_size = 100;
  batch_span->attr("batch.size", static_cast<int64_t>(batch_size));

  for (int i = 0; i < batch_size; ++i) {
    auto item_span = tracer->start_span("process.item", batch_span->context());
    item_span->attr("index", static_cast<int64_t>(i));
    item_span->end();

    metrics.counter("processed").inc();
  }

  metrics.histogram("batch_size").record(static_cast<double>(batch_size));
  batch_span->set_status(obs::StatusCode::Ok);
  batch_span->end();

  SUCCEED();
}

// Long-running operation
TEST_F(IntegrationExtendedTest, LongRunningOperation) {
  auto span = tracer->start_span("long.running.operation");

  for (int checkpoint = 1; checkpoint <= 5; ++checkpoint) {
    span->add_event("checkpoint",
                    {{"number", std::to_string(checkpoint)},
                     {"progress", std::to_string(checkpoint * 20) + "%"}});

    obs::info("Checkpoint reached",
              {{"checkpoint", std::to_string(checkpoint)}});

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  span->set_status(obs::StatusCode::Ok);
  span->end();

  SUCCEED();
}

// Multiple components interaction
TEST_F(IntegrationExtendedTest, MultipleComponentsInteraction) {
  auto root = tracer->start_span("request");

  {
    auto auth = tracer->start_span("auth.validate", root->context());
    auth->kind(obs::SpanKind::Internal);
    obs::debug("Validating auth token");
    auth->set_status(obs::StatusCode::Ok);
    auth->end();
  }

  {
    auto cache = tracer->start_span("cache.lookup", root->context());
    cache->kind(obs::SpanKind::Client);
    obs::debug("Looking up cache");
    cache->set_status(obs::StatusCode::Ok);
    cache->end();
  }

  {
    auto db = tracer->start_span("db.query", root->context());
    db->kind(obs::SpanKind::Client);
    obs::debug("Querying database");
    db->set_status(obs::StatusCode::Ok);
    db->end();
  }

  root->set_status(obs::StatusCode::Ok);
  root->end();

  SUCCEED();
}

// High throughput simulation (1K ops)
TEST_F(IntegrationExtendedTest, HighThroughputSimulation) {
  obs::MetricsRegistry metrics;
  metrics.counter("ops", "high.throughput.ops");

  auto start = std::chrono::steady_clock::now();

  for (int i = 0; i < 1000; ++i) {
    auto span = tracer->start_span("fast.op");
    span->attr("index", static_cast<int64_t>(i));
    span->end();
    metrics.counter("ops").inc();
  }

  auto duration = std::chrono::steady_clock::now() - start;
  auto ops_per_sec = 1000.0 / std::chrono::duration<double>(duration).count();

  obs::info("Throughput test", {{"ops_per_sec", std::to_string(ops_per_sec)}});

  SUCCEED();
}

// Mixed observability workload
TEST_F(IntegrationExtendedTest, MixedObservabilityWorkload) {
  obs::MetricsRegistry metrics;
  metrics.counter("requests", "requests")
      .histogram("latency", "latency")
      .gauge("active", "active");

  for (int i = 0; i < 100; ++i) {
    auto span = tracer->start_span("mixed.op");

    metrics.counter("requests").inc();
    metrics.gauge("active").add(1);

    obs::debug("Processing", {{"iteration", std::to_string(i)}});

    auto latency = static_cast<double>(i % 50);
    metrics.histogram("latency").record(latency);

    span->set_status(obs::StatusCode::Ok);
    span->end();

    metrics.gauge("active").add(-1);
  }

  SUCCEED();
}

// Nested service calls
TEST_F(IntegrationExtendedTest, NestedServiceCalls) {
  auto api1 = tracer->start_span("api.service1");
  api1->kind(obs::SpanKind::Server);

  obs::info("Service 1 called");

  {
    auto svc2_client = tracer->start_span("call.service2", api1->context());
    svc2_client->kind(obs::SpanKind::Client);

    auto svc2_ctx = svc2_client->context();

    // Simulate cross-service call
    {
      auto api2 = tracer->start_span("api.service2", svc2_ctx);
      api2->kind(obs::SpanKind::Server);

      obs::info("Service 2 called");

      {
        auto db = tracer->start_span("db.service2", api2->context());
        db->kind(obs::SpanKind::Client);
        obs::debug("Service 2 DB query");
        db->set_status(obs::StatusCode::Ok);
        db->end();
      }

      api2->set_status(obs::StatusCode::Ok);
      api2->end();
    }

    svc2_client->set_status(obs::StatusCode::Ok);
    svc2_client->end();
  }

  api1->set_status(obs::StatusCode::Ok);
  api1->end();

  SUCCEED();
}

// Observability overhead measurement (benchmark/sanity check)
// This measures overhead and reports it. The threshold is intentionally
// conservative to allow for sanitizer/debug builds - true performance testing
// should use dedicated benchmarks.
TEST_F(IntegrationExtendedTest, ObservabilityOverheadMeasurement) {
  constexpr int iterations = 1000;

  // Measure observability operations
  auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < iterations; ++i) {
    auto span = tracer->start_span("overhead.test");
    auto counter = obs::counter("overhead.counter");
    counter.inc();
    obs::debug("Operation");
    span->end();
  }
  auto duration = std::chrono::steady_clock::now() - start;

  auto total_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
  auto overhead_per_op = total_ns / static_cast<double>(iterations);
  auto overhead_us = overhead_per_op / 1000.0;

  // Report the measurement (always useful for tracking)
  std::cout << "[BENCHMARK] Observability overhead: " << overhead_us
            << " μs/op " << "(" << total_ns / 1000000.0 << " ms for "
            << iterations << " ops)" << std::endl;

  obs::info("Overhead per operation", {{"us", std::to_string(overhead_us)}});

  // Conservative threshold: 500 μs/op catches extreme regressions only
  // Normal release builds: ~15-30 μs
  // Debug builds: ~50-80 μs
  // Sanitizer builds: ~100-200 μs
  // Anything > 500 μs indicates a serious problem
  EXPECT_LT(overhead_per_op, 500000) << "Observability overhead exceeds 500 "
                                        "μs/op - possible severe regression";
}

// Real-world API endpoint simulation
TEST_F(IntegrationExtendedTest, RealWorldAPIEndpoint) {
  obs::MetricsRegistry metrics;
  metrics.counter("requests", "api.requests")
      .counter("errors", "api.errors")
      .duration_histogram("latency", "api.latency")
      .gauge("concurrent", "api.concurrent_requests");

  auto span = tracer->start_span("POST /api/users");
  span->kind(obs::SpanKind::Server)
      .attr("http.method", "POST")
      .attr("http.route", "/api/users")
      .attr("http.scheme", "https");

  metrics.gauge("concurrent").add(1);
  metrics.counter("requests").inc();

  auto start = std::chrono::steady_clock::now();

  obs::info("Request received", {{"method", "POST"},
                                 {"path", "/api/users"},
                                 {"user_agent", "TestClient/1.0"}});

  // Validation
  {
    auto validation = tracer->start_span("validate.request", span->context());
    obs::debug("Validating request body");
    validation->set_status(obs::StatusCode::Ok);
    validation->end();
  }

  // Business logic
  {
    auto logic = tracer->start_span("business.logic", span->context());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    logic->set_status(obs::StatusCode::Ok);
    logic->end();
  }

  // Database
  {
    auto db = tracer->start_span("db.insert", span->context());
    db->kind(obs::SpanKind::Client)
        .attr("db.system", "postgresql")
        .attr("db.operation", "INSERT");

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    db->set_status(obs::StatusCode::Ok);
    db->end();
  }

  auto duration = std::chrono::steady_clock::now() - start;
  metrics.duration_histogram("latency").record(duration);

  span->attr("http.status_code", static_cast<int64_t>(201));
  span->set_status(obs::StatusCode::Ok);
  span->end();

  obs::info("Request completed", {{"status", "201"}});

  metrics.gauge("concurrent").add(-1);

  SUCCEED();
}
