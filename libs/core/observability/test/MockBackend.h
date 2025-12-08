#pragma once
// =============================================================================
// obs/test/MockBackend.h - Shared mock backend for unit tests
// =============================================================================
//
// Usage:
//   #include "MockBackend.h"
//   auto mock = std::make_unique<obs::test::MockBackend>();
//   auto* ptr = mock.get();
//   obs::set_backend(std::move(mock));
//   // ... run tests ...
//   EXPECT_EQ(ptr->span_count.load(), expected);
//
// =============================================================================

#include <obs/IBackend.h>
#include <obs/Span.h>
#include <obs/Metrics.h>
#include <atomic>
#include <vector>
#include <string>
#include <mutex>

namespace obs::test {

/// Thread-safe MockBackend for concurrent unit tests.
/// All operations are no-ops that increment counters.
class ThreadSafeMockBackend : public IBackend {
public:
    // Counters for verification
    std::atomic<int> span_count{0};
    std::atomic<int> log_count{0};
    std::atomic<int> counter_count{0};
    std::atomic<int> histogram_count{0};
    std::atomic<bool> shutdown_called{false};
    
    // Optional: capture names for detailed assertions
    std::mutex names_mutex;
    std::vector<std::string> span_names;
    std::vector<std::string> log_messages;
    
    void shutdown() override { 
        shutdown_called = true; 
    }
    
    std::unique_ptr<Span> create_span(std::string_view name, const Context&) override {
        span_count++;
        {
            std::lock_guard<std::mutex> lock(names_mutex);
            span_names.push_back(std::string(name));
        }
        return nullptr;
    }
    
    std::unique_ptr<Span> create_root_span(std::string_view name) override {
        span_count++;
        {
            std::lock_guard<std::mutex> lock(names_mutex);
            span_names.push_back(std::string(name));
        }
        return nullptr;
    }
    
    void log(Level, std::string_view message, const Context&) override {
        log_count++;
        {
            std::lock_guard<std::mutex> lock(names_mutex);
            log_messages.push_back(std::string(message));
        }
    }
    
    std::shared_ptr<Counter> get_counter(std::string_view, std::string_view) override {
        counter_count++;
        return nullptr;
    }
    
    std::shared_ptr<Histogram> get_histogram(std::string_view, std::string_view) override {
        histogram_count++;
        return nullptr;
    }
    
    std::shared_ptr<Gauge> get_gauge(std::string_view, std::string_view) override {
        return nullptr;
    }
    
    // Helper methods for assertions
    void reset() {
        span_count = 0;
        log_count = 0;
        counter_count = 0;
        histogram_count = 0;
        shutdown_called = false;
        std::lock_guard<std::mutex> lock(names_mutex);
        span_names.clear();
        log_messages.clear();
    }
};

} // namespace obs::test
