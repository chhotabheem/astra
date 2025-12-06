// =============================================================================
// test_context.cpp - Unit tests for obs::Context
// =============================================================================
#include <gtest/gtest.h>
#include <obs/Context.h>

namespace obs::test {

// -----------------------------------------------------------------------------
// TraceId Tests
// -----------------------------------------------------------------------------
TEST(TraceIdTest, DefaultIsInvalid) {
    TraceId id;
    EXPECT_FALSE(id.is_valid());
}

TEST(TraceIdTest, NonZeroIsValid) {
    TraceId id{1, 2};
    EXPECT_TRUE(id.is_valid());
}

TEST(TraceIdTest, ToHexFormat) {
    TraceId id{0x0123456789abcdef, 0xfedcba9876543210};
    std::string hex = id.to_hex();
    EXPECT_EQ(hex.length(), 32);  // 128 bits = 32 hex chars
    EXPECT_EQ(hex, "0123456789abcdeffedcba9876543210");
}

// -----------------------------------------------------------------------------
// SpanId Tests
// -----------------------------------------------------------------------------
TEST(SpanIdTest, DefaultIsInvalid) {
    SpanId id;
    EXPECT_FALSE(id.is_valid());
}

TEST(SpanIdTest, NonZeroIsValid) {
    SpanId id{42};
    EXPECT_TRUE(id.is_valid());
}

TEST(SpanIdTest, ToHexFormat) {
    SpanId id{0x0123456789abcdef};
    std::string hex = id.to_hex();
    EXPECT_EQ(hex.length(), 16);  // 64 bits = 16 hex chars
    EXPECT_EQ(hex, "0123456789abcdef");
}

// -----------------------------------------------------------------------------
// Context Tests
// -----------------------------------------------------------------------------
TEST(ContextTest, DefaultIsInvalid) {
    Context ctx;
    EXPECT_FALSE(ctx.is_valid());
}

TEST(ContextTest, CreateGeneratesValidContext) {
    Context ctx = Context::create();
    EXPECT_TRUE(ctx.is_valid());
    EXPECT_TRUE(ctx.trace_id.is_valid());
}

TEST(ContextTest, CreateGeneratesUniqueTraceIds) {
    Context ctx1 = Context::create();
    Context ctx2 = Context::create();
    
    // Different trace IDs
    EXPECT_NE(ctx1.trace_id.high, ctx2.trace_id.high);
}

TEST(ContextTest, ChildPreservesTraceId) {
    Context parent = Context::create();
    SpanId child_span{123};
    Context child = parent.child(child_span);
    
    EXPECT_EQ(child.trace_id.high, parent.trace_id.high);
    EXPECT_EQ(child.trace_id.low, parent.trace_id.low);
    EXPECT_EQ(child.parent_span_id.value, 123);
}

TEST(ContextTest, ChildPreservesBaggage) {
    Context parent = Context::create();
    parent.baggage["key"] = "value";
    
    Context child = parent.child(SpanId{1});
    
    EXPECT_EQ(child.baggage["key"], "value");
}

// -----------------------------------------------------------------------------
// W3C Trace Context Propagation Tests
// -----------------------------------------------------------------------------
TEST(ContextTest, ToTraceparentFormat) {
    Context ctx;
    ctx.trace_id = TraceId{0x0123456789abcdef, 0xfedcba9876543210};
    ctx.parent_span_id = SpanId{0xaabbccddeeff0011};
    ctx.trace_flags = 0x01;  // Sampled
    
    std::string header = ctx.to_traceparent();
    
    // Format: 00-{trace_id}-{span_id}-{flags}
    EXPECT_EQ(header, "00-0123456789abcdeffedcba9876543210-aabbccddeeff0011-01");
}

TEST(ContextTest, FromTraceparentParsesCorrectly) {
    std::string header = "00-0123456789abcdeffedcba9876543210-aabbccddeeff0011-01";
    
    Context ctx = Context::from_traceparent(header);
    
    EXPECT_TRUE(ctx.is_valid());
    EXPECT_EQ(ctx.trace_id.high, 0x0123456789abcdef);
    EXPECT_EQ(ctx.trace_id.low, 0xfedcba9876543210);
    EXPECT_EQ(ctx.parent_span_id.value, 0xaabbccddeeff0011);
    EXPECT_EQ(ctx.trace_flags, 0x01);
}

TEST(ContextTest, FromTraceparentInvalidReturnsEmpty) {
    Context ctx = Context::from_traceparent("garbage");
    EXPECT_FALSE(ctx.is_valid());
}

TEST(ContextTest, SamplingFlag) {
    Context ctx = Context::create();
    ctx.trace_flags = 0x01;
    EXPECT_TRUE(ctx.is_sampled());
    
    ctx.trace_flags = 0x00;
    EXPECT_FALSE(ctx.is_sampled());
}

} // namespace obs::test
