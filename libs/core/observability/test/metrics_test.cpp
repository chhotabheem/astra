// =============================================================================
// test_metrics.cpp - Unit tests for obs::Counter and obs::Histogram
// =============================================================================
#include <gtest/gtest.h>
#include <obs/IBackend.h>
#include <obs/Metrics.h>
#include <obs/Context.h>
#include <obs/Span.h>
#include <vector>
#include <string>

namespace obs::test {

// -----------------------------------------------------------------------------
// Mock Counter and Histogram
// -----------------------------------------------------------------------------
class MockCounter : public Counter {
public:
    mutable int64_t total = 0;
    mutable int call_count = 0;
    
    void inc() override { total += 1; call_count++; }
    void inc(int64_t value) override { total += value; call_count++; }
    void inc(int64_t value, const Context&) override { total += value; call_count++; }
};

class MockHistogram : public Histogram {
public:
    mutable std::vector<double> recorded;
    
    void record(double value) override { recorded.push_back(value); }
    void record(double value, const Context&) override { recorded.push_back(value); }
};

// -----------------------------------------------------------------------------
// MockBackend for metrics tests
// -----------------------------------------------------------------------------
class MetricsMockBackend : public IBackend {
public:
    std::vector<std::string> counter_names;
    std::vector<std::string> histogram_names;
    std::shared_ptr<MockCounter> last_counter;
    std::shared_ptr<MockHistogram> last_histogram;
    
    void shutdown() override {}
    std::unique_ptr<Span> create_span(std::string_view, const Context&) override { return nullptr; }
    std::unique_ptr<Span> create_root_span(std::string_view) override { return nullptr; }
    void log(Level, std::string_view, const Context&) override {}
    
    std::shared_ptr<Counter> get_counter(std::string_view name, std::string_view) override {
        counter_names.push_back(std::string(name));
        last_counter = std::make_shared<MockCounter>();
        return last_counter;
    }
    
    std::shared_ptr<Histogram> get_histogram(std::string_view name, std::string_view) override {
        histogram_names.push_back(std::string(name));
        last_histogram = std::make_shared<MockHistogram>();
        return last_histogram;
    }
    
    std::shared_ptr<Gauge> get_gauge(std::string_view, std::string_view) override { return nullptr; }
};

// -----------------------------------------------------------------------------
// Metrics Tests
// -----------------------------------------------------------------------------
class MetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock = std::make_unique<MetricsMockBackend>();
        mock_ptr = mock.get();
        obs::set_backend(std::move(mock));
    }
    
    void TearDown() override {
        obs::shutdown();
    }
    
    std::unique_ptr<MetricsMockBackend> mock;
    MetricsMockBackend* mock_ptr;
};

TEST_F(MetricsTest, CounterCreation) {
    auto& counter = obs::counter("http_requests", "Total HTTP requests");
    (void)counter;
    
    ASSERT_EQ(mock_ptr->counter_names.size(), 1);
    EXPECT_EQ(mock_ptr->counter_names[0], "http_requests");
}

TEST_F(MetricsTest, CounterIncrement) {
    auto& counter = obs::counter("requests");
    counter.inc();
    counter.inc(5);
    
    ASSERT_NE(mock_ptr->last_counter, nullptr);
    EXPECT_EQ(mock_ptr->last_counter->total, 6);
    EXPECT_EQ(mock_ptr->last_counter->call_count, 2);
}

TEST_F(MetricsTest, HistogramCreation) {
    auto& hist = obs::histogram("latency", "Request latency");
    (void)hist;
    
    ASSERT_EQ(mock_ptr->histogram_names.size(), 1);
    EXPECT_EQ(mock_ptr->histogram_names[0], "latency");
}

TEST_F(MetricsTest, HistogramRecord) {
    auto& hist = obs::histogram("latency");
    hist.record(0.042);
    hist.record(0.123);
    
    ASSERT_NE(mock_ptr->last_histogram, nullptr);
    ASSERT_EQ(mock_ptr->last_histogram->recorded.size(), 2);
    EXPECT_DOUBLE_EQ(mock_ptr->last_histogram->recorded[0], 0.042);
}

TEST_F(MetricsTest, CounterWithContext) {
    Context ctx = Context::create();
    auto& counter = obs::counter("traced_requests");
    counter.inc(1, ctx);
    
    EXPECT_EQ(mock_ptr->last_counter->total, 1);
}

} // namespace obs::test
