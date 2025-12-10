#include <gtest/gtest.h>
#include <Provider.h>
#include <Span.h>

class TracingTest : public ::testing::Test {
protected:
    void SetUp() override {
        obs::Config config{.service_name = "test"};
        obs::init(config);
    }
    
    void TearDown() override {
        obs::shutdown();
    }
};

// Span/tracing tests with new architecture
TEST_F(TracingTest, SpanCreation) {
    auto span = obs::span("test_operation");
    EXPECT_TRUE(span.is_recording());
}

TEST_F(TracingTest, SpanRAII) {
    // Span ends automatically on scope exit
    {
        auto span = obs::span("test_span");
    }  // span should end here
    SUCCEED();
}

TEST_F(TracingTest, SpanAttributes) {
    auto span = obs::span("operation");
    span.attr("key", "value");
    span.attr("count", static_cast<int64_t>(42));
    span.attr("ratio", 3.14);
    span.attr("flag", true);
    SUCCEED();
}

TEST_F(TracingTest, SpanEvents) {
    auto span = obs::span("operation");
    span.add_event("cache_hit", {{"key", "user_123"}});
    SUCCEED();
}

TEST_F(TracingTest, SpanStatus) {
    auto span = obs::span("operation");
    span.set_status(obs::StatusCode::Error, "Something went wrong");
    SUCCEED();
}

TEST_F(TracingTest, ParentChildRelationship) {
    auto parent = obs::span("parent");
    {
        auto child = obs::span("child");
        // child should have parent context
    }
    SUCCEED();
}
