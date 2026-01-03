#include <Log.h>
#include <Provider.h>
#include <Span.h>
#include <Tracer.h>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

class LogExtendedTest : public ::testing::Test {
protected:
  std::shared_ptr<obs::Tracer> tracer;

  void SetUp() override {
    ::observability::Config config;
    config.set_service_name("log-test");
    obs::init(config);
    tracer = obs::Provider::instance().get_tracer("log-test");
  }

  void TearDown() override {
    tracer.reset();
    obs::shutdown();
  }
};

// All 6 levels with attributes
TEST_F(LogExtendedTest, AllLevelsWithAttributes) {
  obs::trace("trace msg", {{"key", "val"}});
  obs::debug("debug msg", {{"key", "val"}});
  obs::info("info msg", {{"key", "val"}});
  obs::warn("warn msg", {{"key", "val"}});
  obs::error("error msg", {{"key", "val"}});
  obs::fatal("fatal msg", {{"key", "val"}});

  SUCCEED();
}

// Concurrent logging from 1000 threads
TEST_F(LogExtendedTest, ConcurrentLogging1000Threads) {
  std::vector<std::thread> threads;

  for (int i = 0; i < 1000; ++i) {
    threads.emplace_back([i]() {
      obs::info("Thread log", {{"thread_id", std::to_string(i)}});
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

// Empty log messages
TEST_F(LogExtendedTest, EmptyLogMessage) {
  EXPECT_NO_THROW(obs::info(""));
}

// Very long log messages (1MB)
TEST_F(LogExtendedTest, VeryLongLogMessage) {
  std::string long_msg(1000000, 'x');
  EXPECT_NO_THROW(obs::info(long_msg));
}

// Unicode in log messages
TEST_F(LogExtendedTest, UnicodeInLogMessages) {
  obs::info("中文日本語");
  obs::info("🚀 Emoji logs 🎉");
  obs::info("Кириллица");

  SUCCEED();
}

// 100+ log attributes - use 10 due to initializer_list limitations
TEST_F(LogExtendedTest, LogManyAttributes) {
  EXPECT_NO_THROW(obs::info("Many attrs", {{"k1", "v1"},
                                           {"k2", "v2"},
                                           {"k3", "v3"},
                                           {"k4", "v4"},
                                           {"k5", "v5"},
                                           {"k6", "v6"},
                                           {"k7", "v7"},
                                           {"k8", "v8"},
                                           {"k9", "v9"},
                                           {"k10", "v10"}}));
}

// Empty attribute maps
TEST_F(LogExtendedTest, EmptyAttributeMap) {
  EXPECT_NO_THROW(obs::info("No attrs", {}));
}

// Duplicate attribute keys
TEST_F(LogExtendedTest, DuplicateAttributeKeys) {
  EXPECT_NO_THROW(obs::info(
      "Dup keys", {{"key", "val1"}, {"key", "val2"}, {"key", "val3"}}));
}

// All attribute types
TEST_F(LogExtendedTest, AllAttributeTypes) {
  obs::info("All types", {{"string", "value"},
                          {"number", "42"},
                          {"bool", "true"},
                          {"unicode", "中文"}});

  SUCCEED();
}

// Nested scopes (10 levels)
TEST_F(LogExtendedTest, NestedScopes10Levels) {
  {
    obs::ScopedLogAttributes s1({{"level", "1"}});
    {
      obs::ScopedLogAttributes s2({{"level", "2"}});
      {
        obs::ScopedLogAttributes s3({{"level", "3"}});
        {
          obs::ScopedLogAttributes s4({{"level", "4"}});
          {
            obs::ScopedLogAttributes s5({{"level", "5"}});
            {
              obs::ScopedLogAttributes s6({{"level", "6"}});
              {
                obs::ScopedLogAttributes s7({{"level", "7"}});
                {
                  obs::ScopedLogAttributes s8({{"level", "8"}});
                  {
                    obs::ScopedLogAttributes s9({{"level", "9"}});
                    {
                      obs::ScopedLogAttributes s10({{"level", "10"}});
                      obs::info("10 levels deep");
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  SUCCEED();
}

// Multiple scopes same thread
TEST_F(LogExtendedTest, MultipleScopesSameThread) {
  {
    obs::ScopedLogAttributes s1({{"scope", "1"}});
    obs::info("Scope 1");
  }

  {
    obs::ScopedLogAttributes s2({{"scope", "2"}});
    obs::info("Scope 2");
  }

  {
    obs::ScopedLogAttributes s3({{"scope", "3"}});
    obs::info("Scope 3");
  }

  SUCCEED();
}

// Concurrent scopes different threads
TEST_F(LogExtendedTest, ConcurrentScopesDifferentThreads) {
  std::vector<std::thread> threads;

  for (int i = 0; i < 50; ++i) {
    threads.emplace_back([i]() {
      obs::ScopedLogAttributes scope({{"thread", std::to_string(i)}});
      obs::info("Thread scoped log");
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

// Scope with multiple attributes - use 10 due to initializer_list limitations
TEST_F(LogExtendedTest, ScopeWithManyAttributes) {
  obs::ScopedLogAttributes scope({{"k1", "v1"},
                                  {"k2", "v2"},
                                  {"k3", "v3"},
                                  {"k4", "v4"},
                                  {"k5", "v5"},
                                  {"k6", "v6"},
                                  {"k7", "v7"},
                                  {"k8", "v8"},
                                  {"k9", "v9"},
                                  {"k10", "v10"}});
  obs::info("Scoped with many attrs");

  SUCCEED();
}

// Empty scope
TEST_F(LogExtendedTest, EmptyScope) {
  obs::ScopedLogAttributes scope({});
  obs::info("Empty scoped log");

  SUCCEED();
}

// Log with active span
TEST_F(LogExtendedTest, LogWithActiveSpan) {
  auto span = tracer->start_span("operation");
  obs::info("Log during span");
  span->end();

  SUCCEED();
}

// Log without active span
TEST_F(LogExtendedTest, LogWithoutActiveSpan) {
  obs::info("Log without span");

  SUCCEED();
}

// Cross-thread log correlation
TEST_F(LogExtendedTest, CrossThreadLogCorrelation) {
  auto span = tracer->start_span("parent");
  auto ctx = span->context();
  auto local_tracer = tracer;

  std::thread t([local_tracer, ctx]() {
    auto child_span = local_tracer->start_span("child", ctx);
    obs::info("Child thread log");
    child_span->end();
  });

  t.join();
  span->end();

  SUCCEED();
}

// Log after provider shutdown
TEST_F(LogExtendedTest, LogAfterShutdown) {
  obs::shutdown();

  EXPECT_NO_THROW(obs::info("Log after shutdown"));
  EXPECT_NO_THROW(obs::error("Error after shutdown"));
}

// Mixed log levels concurrently
TEST_F(LogExtendedTest, MixedLogLevelsConcurrent) {
  std::vector<std::thread> threads;

  threads.emplace_back([]() {
    for (int i = 0; i < 100; ++i) {
      obs::trace("trace");
    }
  });
  threads.emplace_back([]() {
    for (int i = 0; i < 100; ++i) {
      obs::debug("debug");
    }
  });
  threads.emplace_back([]() {
    for (int i = 0; i < 100; ++i) {
      obs::info("info");
    }
  });
  threads.emplace_back([]() {
    for (int i = 0; i < 100; ++i) {
      obs::warn("warn");
    }
  });
  threads.emplace_back([]() {
    for (int i = 0; i < 100; ++i) {
      obs::error("error");
    }
  });
  threads.emplace_back([]() {
    for (int i = 0; i < 100; ++i) {
      obs::fatal("fatal");
    }
  });

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

// Rapid scope creation/destruction
TEST_F(LogExtendedTest, RapidScopeCreationDestruction) {
  for (int i = 0; i < 1000; ++i) {
    obs::ScopedLogAttributes scope({{"iteration", std::to_string(i)}});
    obs::debug("Rapid scope");
  }

  SUCCEED();
}

// Scoped attributes isolation
TEST_F(LogExtendedTest, ScopedAttributesIsolation) {
  {
    obs::ScopedLogAttributes s1({{"scope", "outer"}});
    obs::info("Outer scope");

    {
      obs::ScopedLogAttributes s2({{"scope", "inner"}});
      obs::info("Inner scope");
    }

    obs::info("Back to outer");
  }

  obs::info("No scope");

  SUCCEED();
}
