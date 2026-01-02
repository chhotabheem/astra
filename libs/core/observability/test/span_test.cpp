#include <Provider.h>
#include <Span.h>
#include <Tracer.h>
#include <gtest/gtest.h>

class SpanTest : public ::testing::Test {
protected:
  std::shared_ptr<obs::Tracer> tracer;

  void SetUp() override {
    ::observability::Config config;
    config.set_service_name("test-service");
    obs::init(config);
    tracer = obs::Provider::instance().get_tracer("test-service");
  }

  void TearDown() override {
    tracer.reset();
    obs::shutdown();
  }
};

TEST_F(SpanTest, BasicSpanCreation) {
  {
    auto span = tracer->start_span("test.operation");
    EXPECT_TRUE(span->is_recording());
    span->end();
  }
}

TEST_F(SpanTest, SpanAttributes) {
  auto span = tracer->start_span("test.operation");

  EXPECT_NO_THROW(span->attr("string_key", "value"));
  EXPECT_NO_THROW(span->attr("int_key", static_cast<int64_t>(42)));
  EXPECT_NO_THROW(span->attr("double_key", 3.14));
  EXPECT_NO_THROW(span->attr("bool_key", true));
  span->end();
}

TEST_F(SpanTest, SpanStatus) {
  auto span = tracer->start_span("test.operation");

  EXPECT_NO_THROW(span->set_status(obs::StatusCode::Ok));
  EXPECT_NO_THROW(span->set_status(obs::StatusCode::Error, "failed"));
  span->end();
}

TEST_F(SpanTest, SpanKind) {
  auto span = tracer->start_span("test.operation");

  EXPECT_NO_THROW(span->kind(obs::SpanKind::Server));
  EXPECT_NO_THROW(span->kind(obs::SpanKind::Client));
  EXPECT_NO_THROW(span->kind(obs::SpanKind::Internal));
  span->end();
}

TEST_F(SpanTest, SpanEvents) {
  auto span = tracer->start_span("test.operation");

  EXPECT_NO_THROW(span->add_event("event1"));
  EXPECT_NO_THROW(span->add_event("event2", {{"key", "value"}}));
  span->end();
}

TEST_F(SpanTest, SpanContextPropagation) {
  auto parent = tracer->start_span("parent");
  auto parent_ctx = parent->context();

  EXPECT_TRUE(parent_ctx.is_valid());
  EXPECT_TRUE(parent_ctx.trace_id.is_valid());
  EXPECT_TRUE(parent_ctx.span_id.is_valid());
  parent->end();
}

TEST_F(SpanTest, AutoParenting) {
  {
    auto parent = tracer->start_span("parent");
    auto parent_ctx = parent->context();

    {
      auto child = tracer->start_span("child", parent_ctx);
      auto child_ctx = child->context();

      // Child should have same trace_id as parent
      EXPECT_EQ(child_ctx.trace_id.high, parent_ctx.trace_id.high);
      EXPECT_EQ(child_ctx.trace_id.low, parent_ctx.trace_id.low);

      // Child should have different span_id
      EXPECT_NE(child_ctx.span_id.value, parent_ctx.span_id.value);
      child->end();
    }
    parent->end();
  }
}

TEST_F(SpanTest, MoveSemantics) {
  auto span1 = tracer->start_span("test");
  auto span2 = std::move(span1);

  EXPECT_TRUE(span2->is_recording());
  span2->end();
}

TEST_F(SpanTest, FluentAPI) {
  auto span = tracer->start_span("test");

  // Should be able to chain calls
  EXPECT_NO_THROW(span->attr("key1", "value1")
                      .attr("key2", static_cast<int64_t>(42))
                      .kind(obs::SpanKind::Server)
                      .add_event("event1")
                      .set_status(obs::StatusCode::Ok));
  span->end();
}

TEST_F(SpanTest, ExplicitEnd) {
  auto span = tracer->start_span("test");
  EXPECT_FALSE(span->is_ended());
  span->end();
  EXPECT_TRUE(span->is_ended());
}

TEST_F(SpanTest, DoubleEndIsNoop) {
  auto span = tracer->start_span("test");
  span->end();
  EXPECT_NO_THROW(span->end()); // Second end should be no-op
}
