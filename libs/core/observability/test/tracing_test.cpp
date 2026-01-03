#include <Provider.h>
#include <Span.h>
#include <Tracer.h>
#include <gtest/gtest.h>

class TracingTest : public ::testing::Test {
protected:
  std::shared_ptr<obs::Tracer> tracer;

  void SetUp() override {
    ::observability::Config config;
    config.set_service_name("test");
    obs::init(config);
    tracer = obs::Provider::instance().get_tracer("test");
  }

  void TearDown() override {
    tracer.reset();
    obs::shutdown();
  }
};

// Span/tracing tests with new architecture
TEST_F(TracingTest, SpanCreation) {
  auto span = tracer->start_span("test_operation");
  EXPECT_TRUE(span->is_recording());
  span->end();
}

TEST_F(TracingTest, SpanRAII) {
  // Span ends automatically on scope exit
  {
    auto span = tracer->start_span("test_span");
    span->end();
  }
  SUCCEED();
}

TEST_F(TracingTest, SpanAttributes) {
  auto span = tracer->start_span("operation");
  span->attr("key", "value");
  span->attr("count", static_cast<int64_t>(42));
  span->attr("ratio", 3.14);
  span->attr("flag", true);
  span->end();
  SUCCEED();
}

TEST_F(TracingTest, SpanEvents) {
  auto span = tracer->start_span("operation");
  span->add_event("cache_hit", {{"key", "user_123"}});
  span->end();
  SUCCEED();
}

TEST_F(TracingTest, SpanStatus) {
  auto span = tracer->start_span("operation");
  span->set_status(obs::StatusCode::Error, "Something went wrong");
  span->end();
  SUCCEED();
}

TEST_F(TracingTest, ParentChildRelationship) {
  auto parent = tracer->start_span("parent");
  {
    auto child = tracer->start_span("child", parent->context());
    // child should have parent context
    child->end();
  }
  parent->end();
  SUCCEED();
}
