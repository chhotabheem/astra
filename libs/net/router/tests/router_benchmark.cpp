#include "Router.h"

#include <benchmark/benchmark.h>
#include <string>

using namespace astra::router;

// =============================================================================
// Match Benchmarks
// =============================================================================

static void BM_MatchStatic(benchmark::State &state) {
  Router router;
  router.get("/users", [](auto, auto) {});
  router.get("/posts", [](auto, auto) {});
  router.get("/comments", [](auto, auto) {});

  for (auto _ : state) {
    auto result = router.match("GET", "/users");
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_MatchStatic);

static void BM_MatchParam(benchmark::State &state) {
  Router router;
  router.get("/users/:id", [](auto, auto) {});

  for (auto _ : state) {
    auto result = router.match("GET", "/users/12345");
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_MatchParam);

static void BM_MatchMultiParam(benchmark::State &state) {
  Router router;
  router.get("/users/:userId/posts/:postId/comments/:commentId",
             [](auto, auto) {});

  for (auto _ : state) {
    auto result = router.match("GET", "/users/123/posts/456/comments/789");
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_MatchMultiParam);

static void BM_MatchManyRoutes(benchmark::State &state) {
  Router router;
  // Add 100 routes
  for (int i = 0; i < 100; ++i) {
    router.get("/route" + std::to_string(i), [](auto, auto) {});
  }

  // Match the last route (worst case)
  for (auto _ : state) {
    auto result = router.match("GET", "/route99");
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_MatchManyRoutes);

static void BM_MatchNotFound(benchmark::State &state) {
  Router router;
  router.get("/users", [](auto, auto) {});

  for (auto _ : state) {
    auto result = router.match("GET", "/nonexistent");
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_MatchNotFound);

// =============================================================================
// Add Route Benchmarks
// =============================================================================

static void BM_AddRoute(benchmark::State &state) {
  for (auto _ : state) {
    Router router;
    router.get("/users/:id", [](auto, auto) {});
    benchmark::DoNotOptimize(router);
  }
}
BENCHMARK(BM_AddRoute);

static void BM_AddManyRoutes(benchmark::State &state) {
  for (auto _ : state) {
    Router router;
    for (int i = 0; i < 100; ++i) {
      router.get("/route" + std::to_string(i), [](auto, auto) {});
    }
    benchmark::DoNotOptimize(router);
  }
}
BENCHMARK(BM_AddManyRoutes);

BENCHMARK_MAIN();
