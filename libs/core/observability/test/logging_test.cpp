#include <Log.h>
#include <Provider.h>
#include <Span.h>
#include <Tracer.h>
#include <gtest/gtest.h>

class LoggingTest : public ::testing::Test {
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
  auto span = tracer->start_span("operation");
  obs::info("Log message");
  span->end();
  SUCCEED();
}

TEST_F(LoggingTest, LogLevels) {
  obs::error("error message");
  obs::fatal("fatal message");
  SUCCEED();
}
