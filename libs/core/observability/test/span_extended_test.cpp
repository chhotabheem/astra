#include <Span.h>
#include <Provider.h>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

class SpanExtendedTest : public ::testing::Test {
protected:
    void SetUp() override {
        obs::Config config{.service_name = "span-test"};
        obs::init(config);
    }
    
    void TearDown() override {
        obs::shutdown();
    }
};

// Create 1000 spans sequentially
TEST_F(SpanExtendedTest, Create1000SpansSequentially) {
    for (int i = 0; i < 1000; ++i) {
        auto span = obs::span("span." + std::to_string(i));
        span.attr("index", static_cast<int64_t>(i));
    }
    SUCCEED();
}

// Create spans without init
TEST_F(SpanExtendedTest, CreateSpanWithoutInit) {
    obs::shutdown();
    auto span = obs::span("no.init");
    EXPECT_NO_THROW(span.attr("key", "value"));
}

// Span with empty name
TEST_F(SpanExtendedTest, SpanWithEmptyName) {
    auto span = obs::span("");
    EXPECT_NO_THROW(span.set_status(obs::StatusCode::Ok));
}

// Span with very long name
TEST_F(SpanExtendedTest, SpanWithVeryLongName) {
    std::string long_name(100000, 'x');
    auto span = obs::span(long_name);
    EXPECT_TRUE(span.is_recording());
}

// Span destruction order (child before parent)
TEST_F(SpanExtendedTest, ChildDestroyedBeforeParent) {
    auto parent = obs::span("parent");
    {
        auto child = obs::span("child");
        child.attr("level", static_cast<int64_t>(1));
    }
    // Child destroyed, parent still alive
    parent.attr("status", "ok");
    SUCCEED();
}

// 100+ attributes on single span
TEST_F(SpanExtendedTest, Span100Attributes) {
    auto span = obs::span("many.attrs");
    
    for (int i = 0; i < 100; ++i) {
        span.attr("key" + std::to_string(i), "value" + std::to_string(i));
    }
    
    SUCCEED();
}

// Duplicate attribute keys
TEST_F(SpanExtendedTest, DuplicateAttributeKeys) {
    auto span = obs::span("dup.keys");
    
    span.attr("key", "value1");
    span.attr("key", "value2");  // Should overwrite
    span.attr("key", "value3");  // Should overwrite again
    
    SUCCEED();
}

// Empty attribute keys
TEST_F(SpanExtendedTest, EmptyAttributeKey) {
    auto span = obs::span("empty.key");
    EXPECT_NO_THROW(span.attr("", "value"));
}

// Attribute value size limits
TEST_F(SpanExtendedTest, HugeAttributeValue) {
    auto span = obs::span("huge.attr");
    std::string huge_value(1000000, 'x');  // 1MB
    EXPECT_NO_THROW(span.attr("huge", huge_value));
}

// All attribute types on one span
TEST_F(SpanExtendedTest, AllAttributeTypesOnSpan) {
    auto span = obs::span("all.types");
    
    span.attr("string", "value");
    span.attr("int", static_cast<int64_t>(42));
    span.attr("double", 3.14);
    span.attr("bool", true);
    span.attr("bool_false", false);
    span.attr("negative_int", static_cast<int64_t>(-100));
    span.attr("zero", static_cast<int64_t>(0));
    
    SUCCEED();
}

// Unicode in attribute values
TEST_F(SpanExtendedTest, UnicodeInAttributes) {
    auto span = obs::span("unicode.attrs");
    
    span.attr("chinese", "中文");
    span.attr("japanese", "日本語");
    span.attr("emoji", "🚀🎉✨");
    span.attr("russian", "Русский");
    
    SUCCEED();
}

// Multiple status changes
TEST_F(SpanExtendedTest, MultipleStatusChanges) {
    auto span = obs::span("status.changes");
    
    span.set_status(obs::StatusCode::Unset);
    span.set_status(obs::StatusCode::Ok);
    span.set_status(obs::StatusCode::Error, "error1");
    span.set_status(obs::StatusCode::Ok);  // Back to ok
    
    SUCCEED();
}

// Empty error message
TEST_F(SpanExtendedTest, EmptyErrorMessage) {
    auto span = obs::span("empty.error");
    EXPECT_NO_THROW(span.set_status(obs::StatusCode::Error, ""));
}

// Very long error message
TEST_F(SpanExtendedTest, VeryLongErrorMessage) {
    auto span = obs::span("long.error");
    std::string long_msg(100000, 'e');
    EXPECT_NO_THROW(span.set_status(obs::StatusCode::Error, long_msg));
}

// Unicode in status message
TEST_F(SpanExtendedTest, UnicodeInStatusMessage) {
    auto span = obs::span("unicode.status");
    EXPECT_NO_THROW(span.set_status(obs::StatusCode::Error, "错误消息"));
}

// 100+ events on single span
TEST_F(SpanExtendedTest, Span100Events) {
    auto span = obs::span("many.events");
    
    for (int i = 0; i < 100; ++i) {
        span.add_event("event" + std::to_string(i));
    }
    
    SUCCEED();
}

// Event with multiple attributes (initializer_list limit)
TEST_F(SpanExtendedTest, EventWith50Attributes) {
    auto span = obs::span("event.attrs");
    
    // Can't easily create 50 with initializer_list, use 5
    EXPECT_NO_THROW(span.add_event("big.event", {
        {"k1", "v1"}, {"k2", "v2"}, {"k3", "v3"}, {"k4", "v4"}, {"k5", "v5"}
    }));
    
    SUCCEED();
}

// Empty event name
TEST_F(SpanExtendedTest, EmptyEventName) {
    auto span = obs::span("empty.event");
    EXPECT_NO_THROW(span.add_event(""));
}

// Duplicate event names
TEST_F(SpanExtendedTest, DuplicateEventNames) {
    auto span = obs::span("dup.events");
    
    span.add_event("event");
    span.add_event("event");
    span.add_event("event");
    
    SUCCEED();
}

// Extract context multiple times
TEST_F(SpanExtendedTest, ExtractContextMultipleTimes) {
    auto span = obs::span("ctx.extract");
    
    auto ctx1 = span.context();
    auto ctx2 = span.context();
    auto ctx3 = span.context();
    
    // Should be same context
    EXPECT_EQ(ctx1.trace_id.high, ctx2.trace_id.high);
    EXPECT_EQ(ctx1.trace_id.low, ctx2.trace_id.low);
    EXPECT_EQ(ctx1.span_id.value, ctx3.span_id.value);
}

// Context propagation across 10 levels
TEST_F(SpanExtendedTest, ContextPropagation10Levels) {
    auto span0 = obs::span("level0");
    auto ctx0 = span0.context();
    
    auto span1 = obs::span("level1");
    auto span2 = obs::span("level2");
    auto span3 = obs::span("level3");
    auto span4 = obs::span("level4");
    auto span5 = obs::span("level5");
    auto span6 = obs::span("level6");
    auto span7 = obs::span("level7");
    auto span8 = obs::span("level8");
    auto span9 = obs::span("level9");
    auto span10 = obs::span("level10");
    
    auto ctx10 = span10.context();
    
    // All should share same trace_id
    EXPECT_EQ(ctx0.trace_id.high, ctx10.trace_id.high);
    EXPECT_EQ(ctx0.trace_id.low, ctx10.trace_id.low);
}

// All span kinds
TEST_F(SpanExtendedTest, AllSpanKinds) {
    auto internal = obs::span("internal");
    internal.kind(obs::SpanKind::Internal);
    
    auto server = obs::span("server");
    server.kind(obs::SpanKind::Server);
    
    auto client = obs::span("client");
    client.kind(obs::SpanKind::Client);
    
    auto producer = obs::span("producer");
    producer.kind(obs::SpanKind::Producer);
    
    auto consumer = obs::span("consumer");
    consumer.kind(obs::SpanKind::Consumer);
    
    SUCCEED();
}

// Span kind after status
TEST_F(SpanExtendedTest, SpanKindAfterStatus) {
    auto span = obs::span("kind.after.status");
    
    span.set_status(obs::StatusCode::Ok);
    span.kind(obs::SpanKind::Server);
    
    SUCCEED();
}

// Concurrent span creation
TEST_F(SpanExtendedTest, ConcurrentSpanCreation100Threads) {
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 100; ++i) {
        threads.emplace_back([i]() {
            for (int j = 0; j < 10; ++j) {
                auto span = obs::span("thread." + std::to_string(i) + ".span." + std::to_string(j));
                span.attr("thread", static_cast<int64_t>(i));
                span.attr("index", static_cast<int64_t>(j));
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    SUCCEED();
}

// Span re-use after move
TEST_F(SpanExtendedTest, SpanAfterMove) {
    auto span1 = obs::span("original");
    auto span2 = std::move(span1);
    
    // span1 should be invalid after move
    // span2 should work
    EXPECT_TRUE(span2.is_recording());
    span2.attr("moved", "yes");
    
    SUCCEED();
}

// Explicit parent with invalid context
TEST_F(SpanExtendedTest, ExplicitParentInvalidContext) {
    obs::Context invalid_ctx;  // Default constructed, invalid
    auto span = obs::span("invalid.parent", invalid_ctx);
    
    // Should create root span
    EXPECT_TRUE(span.is_recording());
}

// Explicit parent with valid context
TEST_F(SpanExtendedTest, ExplicitParentValidContext) {
    auto parent = obs::span("parent");
    auto parent_ctx = parent.context();
    
    auto child = obs::span("child", parent_ctx);
    auto child_ctx = child.context();
    
    // Should share trace_id
    EXPECT_EQ(parent_ctx.trace_id.high, child_ctx.trace_id.high);
    EXPECT_EQ(parent_ctx.trace_id.low, child_ctx.trace_id.low);
}

// Span operations after shutdown
TEST_F(SpanExtendedTest, SpanOperationsAfterShutdown) {
    auto span = obs::span("before.shutdown");
    obs::shutdown();
    
    // Operations should not crash
    EXPECT_NO_THROW(span.attr("key", "value"));
    EXPECT_NO_THROW(span.set_status(obs::StatusCode::Ok));
    EXPECT_NO_THROW(span.add_event("event"));
}
