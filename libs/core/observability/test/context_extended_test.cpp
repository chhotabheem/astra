#include <Context.h>
#include <Provider.h>
#include <Span.h>
#include <Tracer.h>
#include <gtest/gtest.h>
#include <set>
#include <thread>

class ContextExtendedTest : public ::testing::Test {
protected:
  std::shared_ptr<obs::Tracer> tracer;

  void SetUp() override {
    ::observability::Config config;
    config.set_service_name("context-test");
    obs::init(config);
    tracer = obs::Provider::instance().get_tracer("context-test");
  }

  void TearDown() override {
    tracer.reset();
    obs::shutdown();
  }
};

// TraceId generation uniqueness (1000 samples)
TEST_F(ContextExtendedTest, TraceIdUniqueness1000Samples) {
  std::set<std::pair<uint64_t, uint64_t>> trace_ids;

  for (int i = 0; i < 1000; ++i) {
    auto ctx = obs::Context::create();
    auto id_pair = std::make_pair(ctx.trace_id.high, ctx.trace_id.low);

    // Should not have duplicate
    EXPECT_EQ(trace_ids.count(id_pair), 0);
    trace_ids.insert(id_pair);
  }
}

// SpanId generation uniqueness
TEST_F(ContextExtendedTest, SpanIdUniqueness1000Samples) {
  std::set<uint64_t> span_ids;

  for (int i = 0; i < 1000; ++i) {
    auto span = tracer->start_span("test");
    auto ctx = span->context();

    EXPECT_EQ(span_ids.count(ctx.span_id.value), 0);
    span_ids.insert(ctx.span_id.value);
    span->end();
  }
}

// Context equality
TEST_F(ContextExtendedTest, ContextEquality) {
  auto ctx1 = obs::Context::create();
  auto ctx2 = ctx1; // Copy

  EXPECT_EQ(ctx1.trace_id.high, ctx2.trace_id.high);
  EXPECT_EQ(ctx1.trace_id.low, ctx2.trace_id.low);
  EXPECT_EQ(ctx1.span_id.value, ctx2.span_id.value);
  EXPECT_EQ(ctx1.trace_flags, ctx2.trace_flags);
}

// Invalid trace_id edge cases
TEST_F(ContextExtendedTest, InvalidTraceId) {
  obs::Context ctx;
  ctx.trace_id.high = 0;
  ctx.trace_id.low = 0;

  EXPECT_FALSE(ctx.is_valid());
}

// Invalid span_id edge cases
TEST_F(ContextExtendedTest, InvalidSpanId) {
  obs::SpanId span_id;
  span_id.value = 0;

  EXPECT_FALSE(span_id.is_valid());
}

// Baggage with 100+ items
TEST_F(ContextExtendedTest, Baggage100Items) {
  auto ctx = obs::Context::create();

  for (int i = 0; i < 100; ++i) {
    ctx.baggage["key" + std::to_string(i)] = "value" + std::to_string(i);
  }

  EXPECT_EQ(ctx.baggage.size(), 100);
}

// Baggage key/value size limits
TEST_F(ContextExtendedTest, BaggageHugeValues) {
  auto ctx = obs::Context::create();

  std::string huge_key(10000, 'k');
  std::string huge_value(100000, 'v');

  ctx.baggage[huge_key] = huge_value;

  EXPECT_EQ(ctx.baggage[huge_key], huge_value);
}

// Baggage with special characters
TEST_F(ContextExtendedTest, BaggageSpecialCharacters) {
  auto ctx = obs::Context::create();

  ctx.baggage["key!@#$%"] = "value^&*()";
  ctx.baggage["key with spaces"] = "value with spaces";
  ctx.baggage["key=equals"] = "value=equals";

  EXPECT_EQ(ctx.baggage.size(), 3);
}

// Traceparent format validation
TEST_F(ContextExtendedTest, TraceparentValidFormat) {
  auto ctx = obs::Context::create();
  auto traceparent = ctx.to_traceparent();

  // Should be 55 characters: 00-{32}-{16}-{2}
  EXPECT_EQ(traceparent.length(), 55);
  EXPECT_EQ(traceparent[0], '0');
  EXPECT_EQ(traceparent[1], '0');
  EXPECT_EQ(traceparent[2], '-');
}

// Traceparent parsing edge cases
TEST_F(ContextExtendedTest, TraceparentParsingInvalid) {
  // Too short
  auto ctx1 = obs::Context::from_traceparent("short");
  EXPECT_FALSE(ctx1.is_valid());

  // Invalid version
  auto ctx2 = obs::Context::from_traceparent(
      "01-00000000000000000000000000000000-0000000000000000-00");
  EXPECT_FALSE(ctx2.is_valid());

  // Missing delimiters
  auto ctx3 = obs::Context::from_traceparent(
      "00000000000000000000000000000000000000000000000000000");
  EXPECT_FALSE(ctx3.is_valid());
}

// Child context chain (10 levels)
TEST_F(ContextExtendedTest, ChildContextChain10Levels) {
  auto ctx0 = obs::Context::create();
  auto ctx1 = ctx0.child(obs::SpanId{1});
  auto ctx2 = ctx1.child(obs::SpanId{2});
  auto ctx3 = ctx2.child(obs::SpanId{3});
  auto ctx4 = ctx3.child(obs::SpanId{4});
  auto ctx5 = ctx4.child(obs::SpanId{5});
  auto ctx6 = ctx5.child(obs::SpanId{6});
  auto ctx7 = ctx6.child(obs::SpanId{7});
  auto ctx8 = ctx7.child(obs::SpanId{8});
  auto ctx9 = ctx8.child(obs::SpanId{9});
  auto ctx10 = ctx9.child(obs::SpanId{10});

  // All should share same trace_id
  EXPECT_EQ(ctx0.trace_id.high, ctx10.trace_id.high);
  EXPECT_EQ(ctx0.trace_id.low, ctx10.trace_id.low);

  // But different span_ids
  EXPECT_NE(ctx0.span_id.value, ctx10.span_id.value);
}

// Context flags manipulation
TEST_F(ContextExtendedTest, ContextFlagsManipulation) {
  auto ctx = obs::Context::create();

  ctx.set_sampled(true);
  EXPECT_TRUE(ctx.is_sampled());

  ctx.set_sampled(false);
  EXPECT_FALSE(ctx.is_sampled());

  ctx.set_sampled(true);
  EXPECT_TRUE(ctx.is_sampled());
}

// Sampled vs unsampled
TEST_F(ContextExtendedTest, SampledVsUnsampled) {
  auto ctx1 = obs::Context::create();
  ctx1.set_sampled(true);

  auto ctx2 = obs::Context::create();
  ctx2.set_sampled(false);

  EXPECT_NE(ctx1.trace_flags, ctx2.trace_flags);
}

// Context serialization roundtrip
TEST_F(ContextExtendedTest, ContextSerializationRoundtrip) {
  auto original = obs::Context::create();
  original.span_id.value = 12345;
  original.set_sampled(true);

  auto traceparent = original.to_traceparent();
  auto restored = obs::Context::from_traceparent(traceparent);

  EXPECT_EQ(original.trace_id.high, restored.trace_id.high);
  EXPECT_EQ(original.trace_id.low, restored.trace_id.low);
  EXPECT_EQ(original.span_id.value, restored.span_id.value);
  EXPECT_EQ(original.trace_flags, restored.trace_flags);
}

// Baggage header roundtrip
TEST_F(ContextExtendedTest, BaggageHeaderRoundtrip) {
  auto ctx = obs::Context::create();
  ctx.baggage["key1"] = "value1";
  ctx.baggage["key2"] = "value2";

  auto header = ctx.to_baggage_header();

  obs::Context ctx2;
  obs::Context::parse_baggage(ctx2, header);

  EXPECT_EQ(ctx.baggage.size(), ctx2.baggage.size());
  EXPECT_EQ(ctx.baggage["key1"], ctx2.baggage["key1"]);
  EXPECT_EQ(ctx.baggage["key2"], ctx2.baggage["key2"]);
}

// TraceId hex conversion
TEST_F(ContextExtendedTest, TraceIdHexConversion) {
  auto ctx = obs::Context::create();
  auto hex = ctx.trace_id.to_hex();

  // Should be 32 hex characters
  EXPECT_EQ(hex.length(), 32);
}

// SpanId hex conversion
TEST_F(ContextExtendedTest, SpanIdHexConversion) {
  auto span = tracer->start_span("test");
  auto ctx = span->context();
  auto hex = ctx.span_id.to_hex();

  // Should be 16 hex characters
  EXPECT_EQ(hex.length(), 16);
  span->end();
}

// Empty baggage header
TEST_F(ContextExtendedTest, EmptyBaggageHeader) {
  auto ctx = obs::Context::create();
  auto header = ctx.to_baggage_header();

  EXPECT_EQ(header, "");
}

// Baggage parsing empty string
TEST_F(ContextExtendedTest, BaggageParsingEmptyString) {
  obs::Context ctx;
  obs::Context::parse_baggage(ctx, "");

  EXPECT_EQ(ctx.baggage.size(), 0);
}

// Concurrent context creation
TEST_F(ContextExtendedTest, ConcurrentContextCreation) {
  std::vector<std::thread> threads;
  std::vector<obs::Context> contexts(100);

  for (int i = 0; i < 100; ++i) {
    threads.emplace_back([i, &contexts]() {
      contexts[i] = obs::Context::create();
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  // All should be valid and unique
  for (int i = 0; i < 100; ++i) {
    EXPECT_TRUE(contexts[i].is_valid());
  }
}
