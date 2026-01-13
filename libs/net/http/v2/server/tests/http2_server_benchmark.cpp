#include "Http2Request.h"
#include "Http2Response.h"
#include "Http2Server.h"
#include "HttpMethod.h"
#include "Router.h"

#include <benchmark/benchmark.h>
#include <memory>
#include <thread>

using namespace astra::http2;
using namespace astra::router;

static void BM_RouterDispatch(benchmark::State &state) {
  Router router;
  router.add(HttpMethod::GET, "/users/:id", [](auto req, auto res) {
    res->set_status(200);
    res->write("OK");
    res->close();
  });
  router.add(HttpMethod::GET, "/posts/:postId/comments/:commentId",
             [](auto req, auto res) {
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

static void BM_RegisterHandlers(benchmark::State &state) {
  for (auto _ : state) {
    ::http2::ServerConfig config;
    config.set_uri("http://127.0.0.1:0");
    config.set_thread_count(1);

    Router router;
    Http2Server server(config, router);
    server.handle("GET", "/users", [](auto, auto) {});
    server.handle("GET", "/users/:id", [](auto, auto) {});
    server.handle("POST", "/users", [](auto, auto) {});
    server.handle("PUT", "/users/:id", [](auto, auto) {});
    server.handle("DELETE", "/users/:id", [](auto, auto) {});

    benchmark::DoNotOptimize(server);
  }
}
BENCHMARK(BM_RegisterHandlers);

static void BM_ServerConstruction(benchmark::State &state) {
  for (auto _ : state) {
    ::http2::ServerConfig config;
    config.set_uri("http://127.0.0.1:0");
    config.set_thread_count(4);

    Router router;
    Http2Server server(config, router);
    benchmark::DoNotOptimize(server);
  }
}
BENCHMARK(BM_ServerConstruction);

BENCHMARK_MAIN();
