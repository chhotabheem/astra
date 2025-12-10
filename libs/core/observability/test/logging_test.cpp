#include <gtest/gtest.h>
#include <Provider.h>
#include <Log.h>
#include <Span.h>

class LoggingTest : public ::testing::Test {
protected:
    void SetUp() override {
        obs::Config config{.service_name = "test"};
        obs::init(config);
    }
    
    void TearDown() override {
        obs::shutdown();
    }
};

// Logging tests with new architecture
TEST_F(LoggingTest, BasicLog) {
    // Should not crash
    obs::info("Test message");
    SUCCEED();
}

TEST_F(LoggingTest, StructuredAttributes) {
    // Logging with attributes
    obs::info("User login", {{"user_id", "123"}});
    SUCCEED();
}

TEST_F(LoggingTest, AutomaticTraceContext) {
    // Log within a span - should auto-correlate
    auto span = obs::span("operation");
    obs::info("Log message");
    SUCCEED();
}

TEST_F(LoggingTest, LogLevels) {
    obs::error("error message");
    obs::fatal("fatal message");
    SUCCEED();
}
