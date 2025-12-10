#include <Span.h>
#include <Log.h>
#include <Metrics.h>
#include <MetricsRegistry.h>
#include <Provider.h>
#include <gtest/gtest.h>
#include <chrono>
#include <thread>

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        obs::Config config{
            .service_name = "integration-test",
            .service_version = "1.0.0"
        };
        obs::init(config);
    }
    
    void TearDown() override {
        obs::shutdown();
    }
};

TEST_F(IntegrationTest, FullObservabilityStack) {
    // Simulate a complete request with metrics, tracing, and logging
    
    auto request_counter = obs::register_counter("test.requests");
    auto latency_hist = obs::register_duration_histogram("test.latency");
    
    {
        obs::ScopedLogAttributes scoped({
            {"request.id", "req-12345"},
            {"client.ip", "192.168.1.1"}
        });
        
        auto span = obs::span("handle_request");
        span.kind(obs::SpanKind::Server);
        span.attr("http.method", "GET");
        span.attr("http.route", "/api/test");
        
        auto start = std::chrono::steady_clock::now();
        
        obs::info("Request started");
        request_counter.inc();
        
        // Simulate work
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        span.add_event("processing_started");
        
        {
            auto db_span = obs::span("database_query");
            db_span.kind(obs::SpanKind::Client);
            db_span.attr("db.system", "postgresql");
            
            obs::debug("Executing database query");
            
            // Simulate DB work
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            
            db_span.set_status(obs::StatusCode::Ok);
        }
        
        auto duration = std::chrono::steady_clock::now() - start;
        latency_hist.record(duration);
        
        span.set_status(obs::StatusCode::Ok);
        obs::info("Request completed", {
            {"duration_ms", std::to_string(
                std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
            )}
        });
    }
    
    // All spans auto-ended, metrics recorded, logs emitted
}

TEST_F(IntegrationTest, ErrorHandling) {
    auto error_counter = obs::register_counter("test.errors");
    
    {
        auto span = obs::span("failing_operation");
        
        try {
            obs::warn("About to fail");
            throw std::runtime_error("Simulated error");
        } catch (const std::exception& e) {
            error_counter.inc();
            span.set_status(obs::StatusCode::Error, e.what());
            obs::error("Operation failed", {
                {"error.type", "runtime_error"},
                {"error.message", e.what()}
            });
        }
    }
}

TEST_F(IntegrationTest, MetricsRegistryWithSpansAndLogs) {
    obs::MetricsRegistry metrics;
    metrics
        .counter("requests", "test.requests.total")
        .counter("errors", "test.errors.total")
        .duration_histogram("latency", "test.latency")
        .gauge("active", "test.active_requests");
    
    {
        auto span = obs::span("operation");
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
        
        span.set_status(obs::StatusCode::Ok);
        obs::info("Operation completed");
    }
}

TEST_F(IntegrationTest, NestedSpansWithMetricsAndLogs) {
    auto counter = obs::register_counter("nested.operations");
    
    {
        auto parent_span = obs::span("parent");
        parent_span.attr("level", "parent");
        
        obs::info("Parent operation started");
        counter.inc();
        
        {
            auto child_span = obs::span("child");
            child_span.attr("level", "child");
            
            obs::debug("Child operation started");
            
            {
                auto grandchild_span = obs::span("grandchild");
                grandchild_span.attr("level", "grandchild");
                
                obs::trace("Grandchild operation");
                
                grandchild_span.set_status(obs::StatusCode::Ok);
            }
            
            child_span.set_status(obs::StatusCode::Ok);
        }
        
        parent_span.set_status(obs::StatusCode::Ok);
        obs::info("Parent operation completed");
    }
}

TEST_F(IntegrationTest, MultipleAttributeTypes) {
    auto span = obs::span("typed_attributes");
    
    span.attr("string_attr", "value");
    span.attr("int_attr", static_cast<int64_t>(42));
    span.attr("double_attr", 3.14);
    span.attr("bool_attr", true);
    
    obs::info("Multiple attribute types", {
        {"attr1", "string"},
        {"attr2", "value2"}
    });
    
    span.set_status(obs::StatusCode::Ok);
}
