#include <gtest/gtest.h>
#include "Observability.h"
#include "InMemoryExporters.h"

using namespace observability;

class LoggingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize with no-op for now
        initialize_noop();
    }
    
    void TearDown() override {
        shutdown();
    }
};

// Phase 2 tests - will implement these following TDD
TEST_F(LoggingTest, BasicLog) {
    // TODO: Implement test
    info("Test message");
    // Verify log was created
}

TEST_F(LoggingTest, StructuredAttributes) {
    // TODO: Implement test
    info("User login", {{"user_id", "123"}});
    // Verify attributes are present
}

TEST_F(LoggingTest, AutomaticTraceContext) {
    // TODO: Implement test with active span
    Span span("operation");
    info("Log message");
    // Verify log has trace_id from span
}
TEST_F(LoggingTest, LogLevels) {
    error("error message");
    fatal("fatal message");
}
