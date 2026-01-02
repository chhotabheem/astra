#include <Log.h>
#include <Metrics.h>
#include <Provider.h>
#include <Span.h>
#include <Tracer.h>
#include <atomic>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

class ProviderExtendedTest : public ::testing::Test {
protected:
  void TearDown() override {
    obs::shutdown();
  }
};

// Multiple init/shutdown cycles
TEST_F(ProviderExtendedTest, MultipleInitShutdownCycles) {
  for (int i = 0; i < 10; ++i) {
    ::observability::Config config;
    config.set_service_name("test");
    EXPECT_TRUE(obs::init(config));
    EXPECT_TRUE(obs::shutdown());
  }
}

// Concurrent init from multiple threads
TEST_F(ProviderExtendedTest, ConcurrentInit) {
  std::atomic<int> success_count{0};
  std::vector<std::thread> threads;

  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&success_count]() {
      ::observability::Config config;
      config.set_service_name("test");
      if (obs::init(config)) {
        success_count++;
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  // Only one should succeed (first one), others are idempotent
  EXPECT_GE(success_count.load(), 1);
}

// Init with empty service name
TEST_F(ProviderExtendedTest, InitWithEmptyServiceName) {
  ::observability::Config config;
  config.set_service_name("");
  EXPECT_TRUE(obs::init(config)); // Should handle gracefully
}

// Init with very long service name
TEST_F(ProviderExtendedTest, InitWithLongServiceName) {
  std::string long_name(10000, 'a');
  ::observability::Config config;
  config.set_service_name(long_name);
  EXPECT_TRUE(obs::init(config));
}

// Shutdown without init
TEST_F(ProviderExtendedTest, ShutdownWithoutInit) {
  EXPECT_TRUE(obs::shutdown()); // Should be safe
}

// Shutdown idempotency
TEST_F(ProviderExtendedTest, MultipleShutdowns) {
  ::observability::Config config;
  config.set_service_name("test");
  EXPECT_TRUE(obs::init(config));
  EXPECT_TRUE(obs::shutdown());
  EXPECT_TRUE(obs::shutdown()); // Second shutdown should be safe
  EXPECT_TRUE(obs::shutdown()); // Third shutdown should be safe
}

// Provider state after shutdown
TEST_F(ProviderExtendedTest, OperationsAfterShutdown) {
  ::observability::Config config;
  config.set_service_name("test");
  obs::init(config);
  auto tracer = obs::Provider::instance().get_tracer("test");
  obs::shutdown();

  // Operations after shutdown should not crash
  auto counter = obs::counter("test.counter");
  EXPECT_NO_THROW(counter.inc());

  auto span = tracer->start_span("test");
  EXPECT_NO_THROW(span->attr("key", "value"));
  span->end();
}

// Config with special characters
TEST_F(ProviderExtendedTest, ConfigWithSpecialCharacters) {
  ::observability::Config config;
  config.set_service_name("test-service!@#$%^&*()");
  config.set_service_version("1.0.0-beta+sha.abc123");
  config.set_environment("dev/staging/prod");
  EXPECT_TRUE(obs::init(config));
}

// Concurrent shutdown
TEST_F(ProviderExtendedTest, ConcurrentShutdown) {
  ::observability::Config config;
  config.set_service_name("test");
  obs::init(config);

  std::vector<std::thread> threads;
  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([]() {
      obs::shutdown();
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  // Should not crash
  SUCCEED();
}

// Init during shutdown race condition
TEST_F(ProviderExtendedTest, InitDuringShutdownRace) {
  ::observability::Config config;
  config.set_service_name("test");
  obs::init(config);

  std::thread shutdown_thread([]() {
    obs::shutdown();
  });

  std::thread init_thread([]() {
    ::observability::Config config;
    config.set_service_name("test2");
    obs::init(config);
  });

  shutdown_thread.join();
  init_thread.join();

  // Should not crash
  SUCCEED();
}

// Provider singleton thread safety
TEST_F(ProviderExtendedTest, SingletonThreadSafety) {
  std::vector<std::thread> threads;
  std::vector<obs::Provider *> instances(100);

  for (int i = 0; i < 100; ++i) {
    threads.emplace_back([i, &instances]() {
      instances[i] = &obs::Provider::instance();
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  // All instances should be the same
  for (int i = 1; i < 100; ++i) {
    EXPECT_EQ(instances[0], instances[i]);
  }
}

// Config validation - all fields populated
TEST_F(ProviderExtendedTest, ConfigAllFieldsPopulated) {
  ::observability::Config config;
  config.set_service_name("my-service");
  config.set_service_version("2.0.0");
  config.set_environment("staging");
  EXPECT_TRUE(obs::init(config));
}

// Re-init after failed shutdown
TEST_F(ProviderExtendedTest, ReinitAfterShutdown) {
  ::observability::Config config1;
  config1.set_service_name("service1");
  obs::init(config1);
  obs::shutdown();

  ::observability::Config config2;
  config2.set_service_name("service2");
  EXPECT_TRUE(obs::init(config2));
}

// Config with minimal values
TEST_F(ProviderExtendedTest, ConfigMinimalValues) {
  ::observability::Config config;
  config.set_service_name("s"); // Single character
  EXPECT_TRUE(obs::init(config));
}

// Config with Unicode
TEST_F(ProviderExtendedTest, ConfigWithUnicode) {
  ::observability::Config config;
  config.set_service_name("服务-サービス-सेवा");
  config.set_environment("производство");
  EXPECT_TRUE(obs::init(config));
}

// Thread-local storage cleanup verification
TEST_F(ProviderExtendedTest, ThreadLocalStorageCleanup) {
  ::observability::Config config;
  config.set_service_name("test");
  obs::init(config);

  // Create metrics in thread
  std::thread t([]() {
    auto counter = obs::counter("test.counter");
    counter.inc();
  });
  t.join();

  // Shutdown should clean up TLS
  EXPECT_TRUE(obs::shutdown());
}

// Init with null/default config values
TEST_F(ProviderExtendedTest, InitWithDefaultConfig) {
  ::observability::Config config;
  config.set_service_name("test"); // Only set service name
  EXPECT_TRUE(obs::init(config));
}

// Rapid init/shutdown stress test
TEST_F(ProviderExtendedTest, RapidInitShutdownStress) {
  for (int i = 0; i < 100; ++i) {
    ::observability::Config config;
    config.set_service_name("test");
    obs::init(config);
    obs::shutdown();
  }
  SUCCEED();
}
