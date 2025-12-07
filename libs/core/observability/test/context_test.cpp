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
    EXPECT_EQ(child.span_id.value, 123);
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
    ctx.span_id = SpanId{0xaabbccddeeff0011};
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
    EXPECT_EQ(ctx.span_id.value, 0xaabbccddeeff0011);
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

// -----------------------------------------------------------------------------
// P1: span_id naming (TDD - tests new API)
// -----------------------------------------------------------------------------
TEST(ContextTest, SpanIdFieldExists) {
    Context ctx = Context::create();
    ctx.span_id = SpanId{12345};  // Using span_id, not parent_span_id
    EXPECT_EQ(ctx.span_id.value, 12345);
}

TEST(ContextTest, ChildSetsSpanId) {
    Context parent = Context::create();
    parent.span_id = SpanId{111};
    
    SpanId new_span{222};
    Context child = parent.child(new_span);
    
    // Child's span_id becomes the new_span
    EXPECT_EQ(child.span_id.value, 222);
}

// -----------------------------------------------------------------------------
// P2: Baggage deterministic ordering (TDD)
// -----------------------------------------------------------------------------
TEST(ContextTest, BaggageHeaderIsDeterministic) {
    Context ctx1 = Context::create();
    ctx1.baggage["zebra"] = "last";
    ctx1.baggage["alpha"] = "first";
    ctx1.baggage["middle"] = "mid";
    
    Context ctx2 = Context::create();
    ctx2.baggage["middle"] = "mid";
    ctx2.baggage["alpha"] = "first";
    ctx2.baggage["zebra"] = "last";
    
    // Regardless of insertion order, output should be sorted (alpha, middle, zebra)
    EXPECT_EQ(ctx1.to_baggage_header(), ctx2.to_baggage_header());
    EXPECT_EQ(ctx1.to_baggage_header(), "alpha=first,middle=mid,zebra=last");
}

// -----------------------------------------------------------------------------
// P3: TraceFlags formalization (TDD)
// -----------------------------------------------------------------------------
TEST(ContextTest, TraceFlagsConstants) {
    EXPECT_EQ(TraceFlags::NONE, 0x00);
    EXPECT_EQ(TraceFlags::SAMPLED, 0x01);
}

TEST(ContextTest, SetSampledMethod) {
    Context ctx = Context::create();
    
    ctx.set_sampled(true);
    EXPECT_TRUE(ctx.is_sampled());
    EXPECT_EQ(ctx.trace_flags & TraceFlags::SAMPLED, TraceFlags::SAMPLED);
    
    ctx.set_sampled(false);
    EXPECT_FALSE(ctx.is_sampled());
    EXPECT_EQ(ctx.trace_flags & TraceFlags::SAMPLED, 0);
}

TEST(ContextTest, TraceFlagsDefaultIsNone) {
    Context ctx;
    EXPECT_EQ(ctx.trace_flags, TraceFlags::NONE);
}

} // namespace obs::test
