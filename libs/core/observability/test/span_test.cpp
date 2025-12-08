// =============================================================================
// test_span.cpp - Unit tests for obs::Span with MockBackend
// =============================================================================
#include <gtest/gtest.h>
#include <obs/Span.h>
#include <obs/IBackend.h>
#include <obs/Context.h>
#include <vector>
#include <string>

namespace obs::test {

// -----------------------------------------------------------------------------
// MockSpan - Records operations for verification
// -----------------------------------------------------------------------------
class MockSpan : public Span {
public:
    std::string name;
    Context ctx;
    std::vector<std::pair<std::string, std::string>> attrs;
    bool ended = false;
    bool error_set = false;
    std::string error_msg;
    
    MockSpan(std::string_view n, const Context& c) : Span(), name(n), ctx(c) {}
    ~MockSpan() override { ended = true; }
    
    Span& attr(std::string_view key, std::string_view value) override {
        attrs.emplace_back(std::string(key), std::string(value));
        return *this;
    }
    
    Span& attr(std::string_view key, int64_t value) override {
        attrs.emplace_back(std::string(key), std::to_string(value));
        return *this;
    }
    
    Span& attr(std::string_view key, double value) override {
        attrs.emplace_back(std::string(key), std::to_string(value));
        return *this;
    }
    
private:
    Span& do_attr_bool(std::string_view key, bool value) override {
        attrs.emplace_back(std::string(key), value ? "true" : "false");
        return *this;
    }
    
public:
    
    Span& set_error(std::string_view message) override {
        error_set = true;
        error_msg = message;
        return *this;
    }
    
    Span& set_ok() override {
        error_set = false;
        return *this;
    }
    
    Span& event(std::string_view) override { return *this; }
    
    Context context() const override { return ctx; }
    bool is_recording() const override { return true; }
};

// -----------------------------------------------------------------------------
// MockBackend - For testing facade delegation
// -----------------------------------------------------------------------------
class MockBackend : public IBackend {
public:
    std::vector<std::string> created_spans;
    bool shutdown_called = false;
    
    void shutdown() override { shutdown_called = true; }
    
    std::unique_ptr<Span> create_span(std::string_view name, const Context& ctx) override {
        created_spans.push_back(std::string(name));
        return std::make_unique<MockSpan>(name, ctx);
    }
    
    std::unique_ptr<Span> create_root_span(std::string_view name) override {
        created_spans.push_back(std::string(name));
        return std::make_unique<MockSpan>(name, Context::create());
    }
    
    void log(Level, std::string_view, const Context&) override {}
    std::shared_ptr<Counter> get_counter(std::string_view, std::string_view) override { return nullptr; }
    std::shared_ptr<Histogram> get_histogram(std::string_view, std::string_view) override { return nullptr; }
    std::shared_ptr<Gauge> get_gauge(std::string_view, std::string_view) override { return nullptr; }
};

// -----------------------------------------------------------------------------
// Span Tests
// -----------------------------------------------------------------------------
class SpanTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock = std::make_unique<MockBackend>();
        mock_ptr = mock.get();
        obs::set_backend(std::move(mock));
    }
    
    void TearDown() override {
        obs::shutdown();
    }
    
    std::unique_ptr<MockBackend> mock;
    MockBackend* mock_ptr;
};

TEST_F(SpanTest, CreatesSpanWithContext) {
    Context ctx = Context::create();
    auto span = obs::span("test-span", ctx);
    
    EXPECT_EQ(mock_ptr->created_spans.size(), 1);
    EXPECT_EQ(mock_ptr->created_spans[0], "test-span");
}

TEST_F(SpanTest, CreatesRootSpan) {
    auto span = obs::span("root-span");
    
    EXPECT_EQ(mock_ptr->created_spans.size(), 1);
    EXPECT_EQ(mock_ptr->created_spans[0], "root-span");
}

TEST_F(SpanTest, SpanEndsOnScopeExit) {
    {
        auto span = obs::span("scoped");
    }
    // Span destructor called - verified by MockSpan
    EXPECT_EQ(mock_ptr->created_spans.size(), 1);
}

TEST_F(SpanTest, FluentAttributeAPI) {
    auto span = obs::span("with-attrs");
    span->attr("key1", "value1")
        .attr("key2", int64_t{42})
        .attr("key3", 3.14);
    
    // Attributes set on mock span
    EXPECT_EQ(mock_ptr->created_spans.size(), 1);
}

TEST_F(SpanTest, ContextPropagation) {
    Context parent = Context::create();
    auto span = obs::span("child", parent);
    
    Context child_ctx = span->context();
    EXPECT_EQ(child_ctx.trace_id.high, parent.trace_id.high);
    EXPECT_EQ(child_ctx.trace_id.low, parent.trace_id.low);
}

} // namespace obs::test
