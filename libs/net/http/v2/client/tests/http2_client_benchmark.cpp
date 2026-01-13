#include "Http2Client.h"

#include <atomic>
#include <benchmark/benchmark.h>
#include <memory>

using namespace astra::http2;

// =============================================================================
// Client Construction Benchmarks
// =============================================================================

static void BM_ClientConstruction(benchmark::State &state) {
  ::http2::ClientConfig config;
  config.set_request_timeout_ms(100);

  for (auto _ : state) {
    Http2Client client(config);
    benchmark::DoNotOptimize(client);
  }
}
BENCHMARK(BM_ClientConstruction);

// =============================================================================
// Submit Benchmarks (Error Path - Non-Blocking)
// =============================================================================

static void BM_SubmitSinglePeer(benchmark::State &state) {
  ::http2::ClientConfig config;
  config.set_request_timeout_ms(100);
  Http2Client client(config);

  std::atomic<int> completed{0};

  for (auto _ : state) {
    client.submit("127.0.0.1", 19999, "GET", "/test", "", {},
                  [&completed](auto) {
                    completed++;
                  });
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SubmitSinglePeer);

static void BM_SubmitMultiplePeers(benchmark::State &state) {
  ::http2::ClientConfig config;
  config.set_request_timeout_ms(100);
  Http2Client client(config);

  std::atomic<int> completed{0};
  int port_offset = 0;

  for (auto _ : state) {
    uint16_t port = 19990 + (port_offset++ % 10);
    client.submit("127.0.0.1", port, "GET", "/test", "", {},
                  [&completed](auto) {
                    completed++;
                  });
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SubmitMultiplePeers);

// =============================================================================
// Request Payload Benchmarks
// =============================================================================

static void BM_SubmitWithHeaders(benchmark::State &state) {
  ::http2::ClientConfig config;
  config.set_request_timeout_ms(100);
  Http2Client client(config);

  std::map<std::string, std::string> headers;
  headers["Content-Type"] = "application/json";
  headers["Authorization"] = "Bearer token123";
  headers["X-Request-Id"] = "request-id-12345";

  std::atomic<int> completed{0};

  for (auto _ : state) {
    client.submit("127.0.0.1", 19999, "POST", "/api/data",
                  R"({"key": "value"})", headers, [&completed](auto) {
                    completed++;
                  });
  }

  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SubmitWithHeaders);

static void BM_SubmitLargeBody(benchmark::State &state) {
  ::http2::ClientConfig config;
  config.set_request_timeout_ms(100);
  Http2Client client(config);

  std::string large_body(state.range(0), 'x');
  std::atomic<int> completed{0};

  for (auto _ : state) {
    client.submit("127.0.0.1", 19999, "POST", "/upload", large_body, {},
                  [&completed](auto) {
                    completed++;
                  });
  }

  state.SetItemsProcessed(state.iterations());
  state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_SubmitLargeBody)->Range(1024, 65536);

// =============================================================================
// Response Object Benchmarks
// =============================================================================

static void BM_ResponseConstruction(benchmark::State &state) {
  std::map<std::string, std::string> headers;
  headers["content-type"] = "application/json";
  headers["x-request-id"] = "12345";

  for (auto _ : state) {
    Http2ClientResponse response(200, R"({"status": "ok"})", headers);
    benchmark::DoNotOptimize(response);
  }
}
BENCHMARK(BM_ResponseConstruction);

static void BM_ResponseHeaderLookup(benchmark::State &state) {
  std::map<std::string, std::string> headers;
  for (int i = 0; i < 20; ++i) {
    headers["header-" + std::to_string(i)] = "value-" + std::to_string(i);
  }
  Http2ClientResponse response(200, "", headers);

  for (auto _ : state) {
    auto val = response.header("header-15");
    benchmark::DoNotOptimize(val);
  }
}
BENCHMARK(BM_ResponseHeaderLookup);

BENCHMARK_MAIN();
