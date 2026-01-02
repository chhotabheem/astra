#include "Http2Request.h"
#include "Http2Response.h"
#include "Http2Server.h"
#include "Router.h"

#include <benchmark/benchmark.h>
#include <memory>
#include <thread>

using namespace astra::http2;

// =============================================================================
// Router Dispatch Benchmarks (internal path through server)
// =============================================================================

static void BM_RouterDispatch(benchmark::State &state) {
  astra::router::Router router;
  router.get("/users/:id", [](auto req, auto res) {
    res->set_status(200);
    res->write("OK");
    res->close();
  });
  router.get("/posts/:postId/comments/:commentId", [](auto req, auto res) {
    res->set_status(200);
    res->write("OK");
    res->close();
  });

  for (auto _ : state) {
    auto result = router.match("GET", "/users/12345");
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_RouterDispatch);

// =============================================================================
// Handler Registration Benchmarks
// =============================================================================

static void BM_RegisterHandlers(benchmark::State &state) {
  for (auto _ : state) {
    ServerConfig config;
    config.set_port(0); // Random port
    config.set_thread_count(1);

    Http2Server server(config);
    server.handle("GET", "/users", [](auto, auto) {});
    server.handle("GET", "/users/:id", [](auto, auto) {});
    server.handle("POST", "/users", [](auto, auto) {});
    server.handle("PUT", "/users/:id", [](auto, auto) {});
    server.handle("DELETE", "/users/:id", [](auto, auto) {});

    benchmark::DoNotOptimize(server);
  }
}
BENCHMARK(BM_RegisterHandlers);

// =============================================================================
// Full Server Lifecycle (construction/destruction)
// =============================================================================

static void BM_ServerConstruction(benchmark::State &state) {
  for (auto _ : state) {
    ServerConfig config;
    config.set_port(0);
    config.set_thread_count(4);

    Http2Server server(config);
    benchmark::DoNotOptimize(server);
  }
}
BENCHMARK(BM_ServerConstruction);

BENCHMARK_MAIN();
