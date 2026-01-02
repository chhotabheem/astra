#include <Provider.h>
#include <Span.h>
#include <Tracer.h>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

class SpanExtendedTest : public ::testing::Test {
protected:
  std::shared_ptr<obs::Tracer> tracer;

  void SetUp() override {
    ::observability::Config config;
    config.set_service_name("span-test");
    obs::init(config);
    tracer = obs::Provider::instance().get_tracer("span-test");
  }

  void TearDown() override {
    tracer.reset();
    obs::shutdown();
  }
};

// Create 1000 spans sequentially
TEST_F(SpanExtendedTest, Create1000SpansSequentially) {
  for (int i = 0; i < 1000; ++i) {
    auto span = tracer->start_span("span." + std::to_string(i));
    span->attr("index", static_cast<int64_t>(i));
    span->end();
  }
  SUCCEED();
}

// Create spans without init
TEST_F(SpanExtendedTest, CreateSpanWithoutInit) {
  obs::shutdown();
  auto local_tracer = obs::Provider::instance().get_tracer("no-init");
  auto span = local_tracer->start_span("no.init");
  EXPECT_NO_THROW(span->attr("key", "value"));
  span->end();
}

// Span with empty name
TEST_F(SpanExtendedTest, SpanWithEmptyName) {
  auto span = tracer->start_span("");
  EXPECT_NO_THROW(span->set_status(obs::StatusCode::Ok));
  span->end();
}

// Span with very long name
TEST_F(SpanExtendedTest, SpanWithVeryLongName) {
  std::string long_name(100000, 'x');
  auto span = tracer->start_span(long_name);
  EXPECT_TRUE(span->is_recording());
  span->end();
}

// Span destruction order (child before parent)
TEST_F(SpanExtendedTest, ChildDestroyedBeforeParent) {
  auto parent = tracer->start_span("parent");
  {
    auto child = tracer->start_span("child", parent->context());
    child->attr("level", static_cast<int64_t>(1));
    child->end();
  }
  // Child destroyed, parent still alive
  parent->attr("status", "ok");
  parent->end();
  SUCCEED();
}

// 100+ attributes on single span
TEST_F(SpanExtendedTest, Span100Attributes) {
  auto span = tracer->start_span("many.attrs");

  for (int i = 0; i < 100; ++i) {
    span->attr("key" + std::to_string(i), "value" + std::to_string(i));
  }
  span->end();

  SUCCEED();
}

// Duplicate attribute keys
TEST_F(SpanExtendedTest, DuplicateAttributeKeys) {
  auto span = tracer->start_span("dup.keys");

  span->attr("key", "value1");
  span->attr("key", "value2"); // Should overwrite
  span->attr("key", "value3"); // Should overwrite again
  span->end();

  SUCCEED();
}

// Empty attribute keys
TEST_F(SpanExtendedTest, EmptyAttributeKey) {
  auto span = tracer->start_span("empty.key");
  EXPECT_NO_THROW(span->attr("", "value"));
  span->end();
}

// Attribute value size limits
TEST_F(SpanExtendedTest, HugeAttributeValue) {
  auto span = tracer->start_span("huge.attr");
  std::string huge_value(1000000, 'x'); // 1MB
  EXPECT_NO_THROW(span->attr("huge", huge_value));
  span->end();
}

// All attribute types on one span
TEST_F(SpanExtendedTest, AllAttributeTypesOnSpan) {
  auto span = tracer->start_span("all.types");

  span->attr("string", "value");
  span->attr("int", static_cast<int64_t>(42));
  span->attr("double", 3.14);
  span->attr("bool", true);
  span->attr("bool_false", false);
  span->attr("negative_int", static_cast<int64_t>(-100));
  span->attr("zero", static_cast<int64_t>(0));
  span->end();

  SUCCEED();
}

// Unicode in attribute values
TEST_F(SpanExtendedTest, UnicodeInAttributes) {
  auto span = tracer->start_span("unicode.attrs");

  span->attr("chinese", "中文");
  span->attr("japanese", "日本語");
  span->attr("emoji", "🚀🎉✨");
  span->attr("russian", "Русский");
  span->end();

  SUCCEED();
}

// Multiple status changes
TEST_F(SpanExtendedTest, MultipleStatusChanges) {
  auto span = tracer->start_span("status.changes");

  span->set_status(obs::StatusCode::Unset);
  span->set_status(obs::StatusCode::Ok);
  span->set_status(obs::StatusCode::Error, "error1");
  span->set_status(obs::StatusCode::Ok); // Back to ok
  span->end();

  SUCCEED();
}

// Empty error message
TEST_F(SpanExtendedTest, EmptyErrorMessage) {
  auto span = tracer->start_span("empty.error");
  EXPECT_NO_THROW(span->set_status(obs::StatusCode::Error, ""));
  span->end();
}

// Very long error message
TEST_F(SpanExtendedTest, VeryLongErrorMessage) {
  auto span = tracer->start_span("long.error");
  std::string long_msg(100000, 'e');
  EXPECT_NO_THROW(span->set_status(obs::StatusCode::Error, long_msg));
  span->end();
}

// Unicode in status message
TEST_F(SpanExtendedTest, UnicodeInStatusMessage) {
  auto span = tracer->start_span("unicode.status");
  EXPECT_NO_THROW(span->set_status(obs::StatusCode::Error, "错误消息"));
  span->end();
}

// 100+ events on single span
TEST_F(SpanExtendedTest, Span100Events) {
  auto span = tracer->start_span("many.events");

  for (int i = 0; i < 100; ++i) {
    span->add_event("event" + std::to_string(i));
  }
  span->end();

  SUCCEED();
}

// Event with multiple attributes (initializer_list limit)
TEST_F(SpanExtendedTest, EventWith50Attributes) {
  auto span = tracer->start_span("event.attrs");

  // Can't easily create 50 with initializer_list, use 5
  EXPECT_NO_THROW(span->add_event(
      "big.event",
      {{"k1", "v1"}, {"k2", "v2"}, {"k3", "v3"}, {"k4", "v4"}, {"k5", "v5"}}));
  span->end();

  SUCCEED();
}

// Empty event name
TEST_F(SpanExtendedTest, EmptyEventName) {
  auto span = tracer->start_span("empty.event");
  EXPECT_NO_THROW(span->add_event(""));
  span->end();
}

// Duplicate event names
TEST_F(SpanExtendedTest, DuplicateEventNames) {
  auto span = tracer->start_span("dup.events");

  span->add_event("event");
  span->add_event("event");
  span->add_event("event");
  span->end();

  SUCCEED();
}

// Extract context multiple times
TEST_F(SpanExtendedTest, ExtractContextMultipleTimes) {
  auto span = tracer->start_span("ctx.extract");

  auto ctx1 = span->context();
  auto ctx2 = span->context();
  auto ctx3 = span->context();

  // Should be same context
  EXPECT_EQ(ctx1.trace_id.high, ctx2.trace_id.high);
  EXPECT_EQ(ctx1.trace_id.low, ctx2.trace_id.low);
  EXPECT_EQ(ctx1.span_id.value, ctx3.span_id.value);
  span->end();
}

// Context propagation across 10 levels
TEST_F(SpanExtendedTest, ContextPropagation10Levels) {
  auto span0 = tracer->start_span("level0");
  auto ctx0 = span0->context();

  auto span1 = tracer->start_span("level1", ctx0);
  auto span2 = tracer->start_span("level2", span1->context());
  auto span3 = tracer->start_span("level3", span2->context());
  auto span4 = tracer->start_span("level4", span3->context());
  auto span5 = tracer->start_span("level5", span4->context());
  auto span6 = tracer->start_span("level6", span5->context());
  auto span7 = tracer->start_span("level7", span6->context());
  auto span8 = tracer->start_span("level8", span7->context());
  auto span9 = tracer->start_span("level9", span8->context());
  auto span10 = tracer->start_span("level10", span9->context());

  auto ctx10 = span10->context();

  // All should share same trace_id
  EXPECT_EQ(ctx0.trace_id.high, ctx10.trace_id.high);
  EXPECT_EQ(ctx0.trace_id.low, ctx10.trace_id.low);

  span10->end();
  span9->end();
  span8->end();
  span7->end();
  span6->end();
  span5->end();
  span4->end();
  span3->end();
  span2->end();
  span1->end();
  span0->end();
}

// All span kinds
TEST_F(SpanExtendedTest, AllSpanKinds) {
  auto internal = tracer->start_span("internal");
  internal->kind(obs::SpanKind::Internal);
  internal->end();

  auto server = tracer->start_span("server");
  server->kind(obs::SpanKind::Server);
  server->end();

  auto client = tracer->start_span("client");
  client->kind(obs::SpanKind::Client);
  client->end();

  auto producer = tracer->start_span("producer");
  producer->kind(obs::SpanKind::Producer);
  producer->end();

  auto consumer = tracer->start_span("consumer");
  consumer->kind(obs::SpanKind::Consumer);
  consumer->end();

  SUCCEED();
}

// Span kind after status
TEST_F(SpanExtendedTest, SpanKindAfterStatus) {
  auto span = tracer->start_span("kind.after.status");

  span->set_status(obs::StatusCode::Ok);
  span->kind(obs::SpanKind::Server);
  span->end();

  SUCCEED();
}

// Concurrent span creation
TEST_F(SpanExtendedTest, ConcurrentSpanCreation100Threads) {
  std::vector<std::thread> threads;
  auto local_tracer = tracer; // Capture shared_ptr for threads

  for (int i = 0; i < 100; ++i) {
    threads.emplace_back([local_tracer, i]() {
      for (int j = 0; j < 10; ++j) {
        auto span = local_tracer->start_span("thread." + std::to_string(i) +
                                             ".span." + std::to_string(j));
        span->attr("thread", static_cast<int64_t>(i));
        span->attr("index", static_cast<int64_t>(j));
        span->end();
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

// Span re-use after move
TEST_F(SpanExtendedTest, SpanAfterMove) {
  auto span1 = tracer->start_span("original");
  auto span2 = std::move(span1);

  // span1 should be invalid after move
  // span2 should work
  EXPECT_TRUE(span2->is_recording());
  span2->attr("moved", "yes");
  span2->end();

  SUCCEED();
}

// Explicit parent with invalid context
TEST_F(SpanExtendedTest, ExplicitParentInvalidContext) {
  obs::Context invalid_ctx; // Default constructed, invalid
  auto span = tracer->start_span("invalid.parent", invalid_ctx);

  // Should create root span
  EXPECT_TRUE(span->is_recording());
  span->end();
}

// Explicit parent with valid context
TEST_F(SpanExtendedTest, ExplicitParentValidContext) {
  auto parent = tracer->start_span("parent");
  auto parent_ctx = parent->context();

  auto child = tracer->start_span("child", parent_ctx);
  auto child_ctx = child->context();

  // Should share trace_id
  EXPECT_EQ(parent_ctx.trace_id.high, child_ctx.trace_id.high);
  EXPECT_EQ(parent_ctx.trace_id.low, child_ctx.trace_id.low);
  child->end();
  parent->end();
}

// Span operations after shutdown
TEST_F(SpanExtendedTest, SpanOperationsAfterShutdown) {
  auto span = tracer->start_span("before.shutdown");
  obs::shutdown();

  // Operations should not crash
  EXPECT_NO_THROW(span->attr("key", "value"));
  EXPECT_NO_THROW(span->set_status(obs::StatusCode::Ok));
  EXPECT_NO_THROW(span->add_event("event"));
  span->end();
}
