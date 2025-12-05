#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "Observability.h"

using namespace observability;

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        initialize_noop();
    }
    
    void TearDown() override {
        shutdown();
    }
};

// Phase 2 tests
TEST_F(IntegrationTest, TraceLogCorrelation) {
    // Start a span
    Span span("integration_test_span");
    
    // Log something
    info("Log inside span");
    
    // Verify log has trace context
    // In a real test we'd capture stdout or use a mock exporter
    // For now we just verify it runs without crashing
}

TEST_F(IntegrationTest, FullFlow) {
    // Simulate a request
    Span request_span("handle_request");
    request_span.set_attribute("method", "GET");
    request_span.set_attribute("path", "/api/data");
    
    info("Received request", {{"client_ip", "10.0.0.1"}});
    
    {
        Span db_span("db_query");
        db_span.set_attribute("db.statement", "SELECT * FROM users");
        // Simulate DB work
        info("Querying database");
    }
    
    auto counter = Metrics::create_counter("http_requests");
    counter.increment(1, {{"status", "200"}});
    
    request_span.set_ok();
}

// =============================================================================
// Thread Safety Tests (Phase 6 - for TSan/Helgrind validation)
// =============================================================================

TEST_F(IntegrationTest, ConcurrentSpanCreation) {
    // Multiple threads creating spans simultaneously
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([i] {
            for (int j = 0; j < 100; ++j) {
                Span span("concurrent_span_" + std::to_string(i));
                span.set_attribute("iteration", static_cast<int64_t>(j));
            }
        });
    }
    for (auto& t : threads) t.join();
}

TEST_F(IntegrationTest, ConcurrentLogging) {
    // Multiple threads logging simultaneously
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([i] {
            for (int j = 0; j < 100; ++j) {
                info("Concurrent log", {{"thread", std::to_string(i)}, {"iter", std::to_string(j)}});
            }
        });
    }
    for (auto& t : threads) t.join();
}

TEST_F(IntegrationTest, ConcurrentMetrics) {
    // Multiple threads incrementing same counter
    auto counter = Metrics::create_counter("concurrent_counter");
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&counter] {
            for (int j = 0; j < 100; ++j) {
                counter.increment(1);
            }
        });
    }
    for (auto& t : threads) t.join();
}

TEST_F(IntegrationTest, ConcurrentFullWorkload) {
    // Simulate realistic concurrent request handling
    std::vector<std::thread> threads;
    auto request_counter = Metrics::create_counter("requests");
    auto latency_histogram = Metrics::create_histogram("latency");
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([i, &request_counter, &latency_histogram] {
            for (int j = 0; j < 50; ++j) {
                Span span("handle_request");
                span.set_attribute("thread_id", static_cast<int64_t>(i));
                
                info("Processing request", {{"req_id", std::to_string(i * 100 + j)}});
                
                request_counter.increment(1, {{"status", "200"}});
                latency_histogram.record(0.05 + (j * 0.001));
                
                span.set_ok();
            }
        });
    }
    for (auto& t : threads) t.join();
}
