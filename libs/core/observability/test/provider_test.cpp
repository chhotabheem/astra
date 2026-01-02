#include <Provider.h>
#include <gtest/gtest.h>

namespace {

class ProviderTest : public ::testing::Test {
protected:
  void TearDown() override {
    // Always shutdown after each test
    obs::shutdown();
  }
};

TEST_F(ProviderTest, InitializationDoesNotThrow) {
  ::observability::Config config;
  config.set_service_name("test-service");
  config.set_otlp_endpoint("http://localhost:4317");

  EXPECT_NO_THROW(obs::init(config));
  EXPECT_NO_THROW(obs::shutdown());
}

TEST_F(ProviderTest, CanInitializeMultipleTimes) {
  ::observability::Config config;
  config.set_service_name("test");

  ASSERT_TRUE(obs::init(config));
  ASSERT_TRUE(obs::init(config)); // Should be idempotent

  // Should not throw
}

TEST_F(ProviderTest, ShutdownWithoutInitDoesNotCrash) {
  // Should be safe to call shutdown without init
  EXPECT_NO_THROW(obs::shutdown());
}

TEST_F(ProviderTest, MultipleShutdownsSafe) {
  ::observability::Config config;
  config.set_service_name("test");
  ASSERT_TRUE(obs::init(config));

  obs::shutdown();
  obs::shutdown(); // Should be safe
}

} // namespace
