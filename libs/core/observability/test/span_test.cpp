#include <Span.h>
#include <Provider.h>
#include <gtest/gtest.h>

class SpanTest : public ::testing::Test {
protected:
    void SetUp() override {
        obs::Config config{.service_name = "test-service"};
        obs::init(config);
    }
    
    void TearDown() override {
        obs::shutdown();
    }
};

TEST_F(SpanTest, BasicSpanCreation) {
    {
        auto span = obs::span("test.operation");
        EXPECT_TRUE(span.is_recording());
    }
    // Span should auto-end on destruction
}

TEST_F(SpanTest, SpanAttributes) {
    auto span = obs::span("test.operation");
    
    EXPECT_NO_THROW(span.attr("string_key", "value"));
    EXPECT_NO_THROW(span.attr("int_key", static_cast<int64_t>(42)));
    EXPECT_NO_THROW(span.attr("double_key", 3.14));
    EXPECT_NO_THROW(span.attr("bool_key", true));
}

TEST_F(SpanTest, SpanStatus) {
    auto span = obs::span("test.operation");
    
    EXPECT_NO_THROW(span.set_status(obs::StatusCode::Ok));
    EXPECT_NO_THROW(span.set_status(obs::StatusCode::Error, "failed"));
}

TEST_F(SpanTest, SpanKind) {
    auto span = obs::span("test.operation");
    
    EXPECT_NO_THROW(span.kind(obs::SpanKind::Server));
    EXPECT_NO_THROW(span.kind(obs::SpanKind::Client));
    EXPECT_NO_THROW(span.kind(obs::SpanKind::Internal));
}

TEST_F(SpanTest, SpanEvents) {
    auto span = obs::span("test.operation");
    
    EXPECT_NO_THROW(span.add_event("event1"));
    EXPECT_NO_THROW(span.add_event("event2", {{"key", "value"}}));
}

TEST_F(SpanTest, SpanContextPropagation) {
    auto parent = obs::span("parent");
    auto parent_ctx = parent.context();
    
    EXPECT_TRUE(parent_ctx.is_valid());
    EXPECT_TRUE(parent_ctx.trace_id.is_valid());
    EXPECT_TRUE(parent_ctx.span_id.is_valid());
}

TEST_F(SpanTest, AutoParenting) {
    {
        auto parent = obs::span("parent");
        auto parent_ctx = parent.context();
        
        {
            auto child = obs::span("child");
            auto child_ctx = child.context();
            
            // Child should have same trace_id as parent
            EXPECT_EQ(child_ctx.trace_id.high, parent_ctx.trace_id.high);
            EXPECT_EQ(child_ctx.trace_id.low, parent_ctx.trace_id.low);
            
            // Child should have different span_id
            EXPECT_NE(child_ctx.span_id.value, parent_ctx.span_id.value);
        }
    }
}

TEST_F(SpanTest, MoveSemantics) {
    auto span1 = obs::span("test");
    auto span2 = std::move(span1);
    
    EXPECT_TRUE(span2.is_recording());
}

TEST_F(SpanTest, FluentAPI) {
    auto span = obs::span("test");
    
    // Should be able to chain calls
    EXPECT_NO_THROW(
        span.attr("key1", "value1")
            .attr("key2", static_cast<int64_t>(42))
            .kind(obs::SpanKind::Server)
            .add_event("event1")
            .set_status(obs::StatusCode::Ok)
    );
}
