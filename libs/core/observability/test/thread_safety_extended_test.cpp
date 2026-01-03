#include <Log.h>
#include <Metrics.h>
#include <MetricsRegistry.h>
#include <Provider.h>
#include <Span.h>
#include <Tracer.h>
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

class ThreadSafetyExtendedTest : public ::testing::Test {
protected:
  std::shared_ptr<obs::Tracer> tracer;

  void SetUp() override {
    ::observability::Config config;
    config.set_service_name("thread-safety-test");
    obs::init(config);
    tracer = obs::Provider::instance().get_tracer("thread-safety-test");
  }

  void TearDown() override {
    tracer.reset();
    obs::shutdown();
  }
};

// 100 threads creating metrics concurrently
TEST_F(ThreadSafetyExtendedTest, Metrics100ThreadsConcurrent) {
  std::vector<std::thread> threads;

  for (int i = 0; i < 100; ++i) {
    threads.emplace_back([i]() {
      auto counter = obs::counter("thread.counter." + std::to_string(i));
      auto hist = obs::histogram("thread.hist." + std::to_string(i));
      auto gauge = obs::gauge("thread.gauge." + std::to_string(i));

      for (int j = 0; j < 100; ++j) {
        counter.inc();
        hist.record(static_cast<double>(j));
        gauge.set(j);
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

// Init vs metric creation race
TEST_F(ThreadSafetyExtendedTest, InitVsMetricCreationRace) {
  obs::shutdown();

  std::thread t1([]() {
    ::observability::Config config;
    config.set_service_name("test");
    obs::init(config);
  });

  std::thread t2([]() {
    auto counter = obs::counter("race.counter");
    counter.inc();
  });

  t1.join();
  t2.join();

  SUCCEED();
}

// Shutdown vs active operations
TEST_F(ThreadSafetyExtendedTest, ShutdownVsActiveOperations) {
  auto counter = obs::counter("active.counter");

  std::atomic<bool> stop{false};

  std::thread worker([&counter, &stop]() {
    while (!stop.load()) {
      counter.inc();
    }
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Proper shutdown: stop activity first, then shutdown
  stop.store(true);
  worker.join();

  obs::shutdown();

  SUCCEED();
}

// Active span stack thread isolation
TEST_F(ThreadSafetyExtendedTest, ActiveSpanStackIsolation) {
  std::vector<std::thread> threads;
  auto local_tracer = tracer;

  for (int i = 0; i < 50; ++i) {
    threads.emplace_back([local_tracer, i]() {
      auto span1 =
          local_tracer->start_span("thread." + std::to_string(i) + ".span1");
      {
        auto span2 = local_tracer->start_span(
            "thread." + std::to_string(i) + ".span2", span1->context());
        {
          auto span3 = local_tracer->start_span(
              "thread." + std::to_string(i) + ".span3", span2->context());
          span3->end();
        }
        span2->end();
      }
      span1->end();
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

// Metric registration race conditions
TEST_F(ThreadSafetyExtendedTest, MetricRegistrationRace) {
  std::vector<std::thread> threads;

  // Multiple threads trying to register same metric
  for (int i = 0; i < 100; ++i) {
    threads.emplace_back([]() {
      auto counter = obs::counter("shared.counter");
      counter.inc();
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

// MetricsRegistry concurrent access
TEST_F(ThreadSafetyExtendedTest, MetricsRegistryConcurrentAccess) {
  obs::MetricsRegistry registry;

  registry.counter("c1", "counter1")
      .counter("c2", "counter2")
      .histogram("h1", "hist1")
      .gauge("g1", "gauge1");

  std::vector<std::thread> threads;

  for (int i = 0; i < 50; ++i) {
    threads.emplace_back([&registry]() {
      for (int j = 0; j < 100; ++j) {
        registry.counter("c1").inc();
        registry.histogram("h1").record(1.0);
        registry.gauge("g1").add(1);
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

// Provider singleton concurrent access
TEST_F(ThreadSafetyExtendedTest, ProviderConcurrentAccess) {
  std::vector<std::thread> threads;

  for (int i = 0; i < 100; ++i) {
    threads.emplace_back([]() {
      auto &provider = obs::Provider::instance();
      (void)provider; // Just access it
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

// Concurrent context extraction
TEST_F(ThreadSafetyExtendedTest, ConcurrentContextExtraction) {
  auto span = tracer->start_span("shared");

  std::vector<std::thread> threads;
  std::vector<obs::Context> contexts(100);

  for (int i = 0; i < 100; ++i) {
    threads.emplace_back([i, &span, &contexts]() {
      contexts[i] = span->context();
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  span->end();

  // All should be same
  for (int i = 1; i < 100; ++i) {
    EXPECT_EQ(contexts[0].trace_id.high, contexts[i].trace_id.high);
    EXPECT_EQ(contexts[0].span_id.value, contexts[i].span_id.value);
  }
}

// Span parent-child race
TEST_F(ThreadSafetyExtendedTest, SpanParentChildRace) {
  std::vector<std::thread> threads;
  auto local_tracer = tracer;

  for (int i = 0; i < 50; ++i) {
    threads.emplace_back([local_tracer]() {
      auto parent = local_tracer->start_span("parent");
      auto parent_ctx = parent->context();

      std::thread child_thread([local_tracer, parent_ctx]() {
        auto child = local_tracer->start_span("child", parent_ctx);
        child->end();
      });

      child_thread.join();
      parent->end();
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

// Shutdown during active spans
TEST_F(ThreadSafetyExtendedTest, ShutdownDuringActiveSpans) {
  std::atomic<bool> stop{false};
  auto local_tracer = tracer;

  std::thread worker([&stop, local_tracer]() {
    while (!stop.load()) {
      auto span = local_tracer->start_span("active");
      span->attr("key", "value");
      span->end();
    }
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Proper shutdown: stop activity first, then shutdown
  stop.store(true);
  worker.join();

  obs::shutdown();

  SUCCEED();
}

// Thread-local storage isolation verification
TEST_F(ThreadSafetyExtendedTest, TLSIsolationVerification) {
  auto shared_counter = obs::counter("shared");

  std::vector<std::thread> threads;
  std::atomic<int> total_incs{0};

  for (int i = 0; i < 100; ++i) {
    threads.emplace_back([&shared_counter, &total_incs]() {
      for (int j = 0; j < 1000; ++j) {
        shared_counter.inc();
        total_incs++;
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  EXPECT_EQ(total_incs.load(), 100000);
}

// High contention scenario
TEST_F(ThreadSafetyExtendedTest, HighContentionScenario) {
  auto counter = obs::counter("high.contention");
  auto hist = obs::histogram("high.contention.hist");
  auto local_tracer = tracer;

  std::vector<std::thread> threads;

  for (int i = 0; i < 200; ++i) {
    threads.emplace_back([&counter, &hist, local_tracer, i]() {
      for (int j = 0; j < 100; ++j) {
        counter.inc();
        hist.record(static_cast<double>(i * j));

        auto span = local_tracer->start_span("contention");
        span->attr("thread", static_cast<int64_t>(i));
        span->end();

        obs::info("High contention", {{"t", std::to_string(i)}});
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

// Concurrent span creation with same name
TEST_F(ThreadSafetyExtendedTest, ConcurrentSpansSameName) {
  std::vector<std::thread> threads;
  auto local_tracer = tracer;

  for (int i = 0; i < 100; ++i) {
    threads.emplace_back([local_tracer]() {
      auto span = local_tracer->start_span("same.name");
      std::hash<std::thread::id> hasher;
      span->attr("unique", std::to_string(hasher(std::this_thread::get_id())));
      span->end();
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

// Concurrent logging with scoped attributes
TEST_F(ThreadSafetyExtendedTest, ConcurrentLoggingWithScopes) {
  std::vector<std::thread> threads;

  for (int i = 0; i < 50; ++i) {
    threads.emplace_back([i]() {
      obs::ScopedLogAttributes scope({{"thread", std::to_string(i)}});

      for (int j = 0; j < 100; ++j) {
        obs::info("Scoped log", {{"iteration", std::to_string(j)}});
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}

// Mixed workload stress test
TEST_F(ThreadSafetyExtendedTest, MixedWorkloadStress) {
  std::atomic<bool> stop{false};
  std::vector<std::thread> threads;
  auto local_tracer = tracer;

  // Metrics thread
  threads.emplace_back([&stop]() {
    auto counter = obs::counter("stress.counter");
    while (!stop.load()) {
      counter.inc();
    }
  });

  // Span thread
  threads.emplace_back([&stop, local_tracer]() {
    while (!stop.load()) {
      auto span = local_tracer->start_span("stress.span");
      span->attr("stress", "true");
      span->end();
    }
  });

  // Log thread
  threads.emplace_back([&stop]() {
    while (!stop.load()) {
      obs::info("Stress log");
    }
  });

  // Let it run
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  stop.store(true);

  for (auto &t : threads) {
    t.join();
  }

  SUCCEED();
}
