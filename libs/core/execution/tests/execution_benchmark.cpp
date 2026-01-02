#include "IMessageHandler.h"
#include "Message.h"
#include "SharedQueue.h"
#include "StickyQueue.h"

#include <atomic>
#include <benchmark/benchmark.h>
#include <thread>

using namespace astra::execution;
namespace obs = astra::observability;

// =============================================================================
// Mock Handler
// =============================================================================

class NoOpHandler : public IMessageHandler {
public:
  void handle(Message &msg) override {
    // Real work simulation - parse JSON-like payload
    if (msg.payload.has_value()) {
      auto *data = std::any_cast<std::string>(&msg.payload);
      if (data) {
        benchmark::DoNotOptimize(*data);
      }
    }
  }
};

// =============================================================================
// SharedQueue Benchmarks
// =============================================================================

static void BM_SharedQueueSubmit(benchmark::State &state) {
  NoOpHandler handler;
  SharedQueue queue(4); // 4 workers
  queue.start();

  std::string payload = "test payload data for benchmark";

  for (auto _ : state) {
    queue.submit({1, obs::Context{}, std::any{payload}});
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  queue.stop();
}
BENCHMARK(BM_SharedQueueSubmit);

static void BM_SharedQueueThroughput(benchmark::State &state) {
  NoOpHandler handler;
  SharedQueue queue(4);
  queue.start();

  std::atomic<int> count{0};
  std::string payload = "throughput test";

  for (auto _ : state) {
    for (int i = 0; i < 1000; ++i) {
      queue.submit({static_cast<uint64_t>(i), obs::Context{},
                    std::function<void()>([&count] {
                      count++;
                    })});
    }
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  queue.stop();

  state.SetItemsProcessed(state.iterations() * 1000);
}
BENCHMARK(BM_SharedQueueThroughput);

// =============================================================================
// StickyQueue Benchmarks
// =============================================================================

static void BM_StickyQueueSubmit(benchmark::State &state) {
  NoOpHandler handler;
  StickyQueue queue(4, handler);
  queue.start();

  std::string payload = "test payload data";

  for (auto _ : state) {
    Message msg{1, obs::Context{}, std::any{payload}};
    queue.submit(std::move(msg));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  queue.stop();
}
BENCHMARK(BM_StickyQueueSubmit);

static void BM_StickyQueueAffinity(benchmark::State &state) {
  NoOpHandler handler;
  StickyQueue queue(4, handler);
  queue.start();

  // Submit to same session - tests affinity
  for (auto _ : state) {
    for (int i = 0; i < 100; ++i) {
      Message msg{42, obs::Context{}, std::any{"session affinity test"}};
      queue.submit(std::move(msg));
    }
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  queue.stop();

  state.SetItemsProcessed(state.iterations() * 100);
}
BENCHMARK(BM_StickyQueueAffinity);

static void BM_StickyQueueDistribution(benchmark::State &state) {
  NoOpHandler handler;
  StickyQueue queue(4, handler);
  queue.start();

  // Submit to different sessions - tests distribution
  for (auto _ : state) {
    for (int i = 0; i < 100; ++i) {
      Message msg{static_cast<uint64_t>(i), obs::Context{},
                  std::any{"distribution test"}};
      queue.submit(std::move(msg));
    }
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  queue.stop();

  state.SetItemsProcessed(state.iterations() * 100);
}
BENCHMARK(BM_StickyQueueDistribution);

BENCHMARK_MAIN();
