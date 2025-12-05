#include <gtest/gtest.h>
#include "Observability.h"

using namespace observability;

class TracingTest : public ::testing::Test {
protected:
    void SetUp() override {
        initialize_noop();
    }
    
    void TearDown() override {
        shutdown();
    }
};

// Phase 2 tests
TEST_F(TracingTest, SpanCreation) {
    // TODO: Implement test
    Span span("test_operation");
    //Verify span was created
}

TEST_F(TracingTest, SpanRAII) {
    // TODO: Verify span ends automatically
    {
        Span span("test_span");
    }  // span should end here
}

TEST_F(TracingTest, SpanAttributes) {
    // TODO: Test setting attributes
    Span span("operation");
    span.set_attribute("key", "value");
    span.set_attribute("count", static_cast<int64_t>(42));
    span.set_attribute("ratio", 3.14);
    span.set_attribute("flag", true);
}

TEST_F(TracingTest, SpanEvents) {
    // TODO: Test adding events
    Span span("operation");
    span.add_event("cache_hit", {{"key", "user_123"}});
}

TEST_F(TracingTest, SpanError) {
    // TODO: Test error status
    Span span("operation");
    span.set_error("Something went wrong");
}

TEST_F(TracingTest, ParentChildRelationship) {
    // TODO: Test span hierarchy
    Span parent("parent");
    {
        Span child("child");
        // child should have parent as parent_span_id
    }
}
