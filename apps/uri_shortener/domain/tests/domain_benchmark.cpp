#include "ExpirationPolicy.h"
#include "OriginalUrl.h"
#include "ShortCode.h"
#include "ShortLink.h"

#include <benchmark/benchmark.h>
#include <string>

using namespace uri_shortener::domain;

// =============================================================================
// ShortCode Benchmarks
// =============================================================================

static void BM_ShortCodeCreate(benchmark::State &state) {
  int i = 0;
  for (auto _ : state) {
    auto code = ShortCode::create("abc" + std::to_string(i++ % 1000));
    benchmark::DoNotOptimize(code);
  }
}
BENCHMARK(BM_ShortCodeCreate);

static void BM_ShortCodeValidation(benchmark::State &state) {
  auto code = ShortCode::create("validcode123");

  for (auto _ : state) {
    auto result = ShortCode::create("test1234");
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_ShortCodeValidation);

// =============================================================================
// OriginalUrl Benchmarks
// =============================================================================

static void BM_OriginalUrlCreate(benchmark::State &state) {
  for (auto _ : state) {
    auto url = OriginalUrl::create(
        "https://example.com/very/long/path/to/some/resource");
    benchmark::DoNotOptimize(url);
  }
}
BENCHMARK(BM_OriginalUrlCreate);

static void BM_OriginalUrlValidation(benchmark::State &state) {
  // Invalid URLs
  std::vector<std::string> invalid_urls = {
      "not-a-url",
      "ftp://invalid",
      "",
      "http://",
  };

  int i = 0;
  for (auto _ : state) {
    auto url = OriginalUrl::create(invalid_urls[i % invalid_urls.size()]);
    benchmark::DoNotOptimize(url);
    i++;
  }
}
BENCHMARK(BM_OriginalUrlValidation);

// =============================================================================
// ShortLink Benchmarks
// =============================================================================

static void BM_ShortLinkCreate(benchmark::State &state) {
  auto code = ShortCode::create("testcode").value();
  auto url = OriginalUrl::create("https://example.com").value();

  for (auto _ : state) {
    auto link = ShortLink::create(code, url, ExpirationPolicy::never());
    benchmark::DoNotOptimize(link);
  }
}
BENCHMARK(BM_ShortLinkCreate);

static void BM_ShortLinkWithExpiry(benchmark::State &state) {
  auto code = ShortCode::create("testcode").value();
  auto url = OriginalUrl::create("https://example.com").value();
  auto expiry = ExpirationPolicy::after(std::chrono::hours(24));

  for (auto _ : state) {
    auto link = ShortLink::create(code, url, expiry);
    benchmark::DoNotOptimize(link);
  }
}
BENCHMARK(BM_ShortLinkWithExpiry);

static void BM_ShortLinkIsExpired(benchmark::State &state) {
  auto code = ShortCode::create("testcode").value();
  auto url = OriginalUrl::create("https://example.com").value();
  auto expiry = ExpirationPolicy::after(std::chrono::hours(24));
  auto link = ShortLink::create(code, url, expiry).value();

  for (auto _ : state) {
    bool expired = link.is_expired();
    benchmark::DoNotOptimize(expired);
  }
}
BENCHMARK(BM_ShortLinkIsExpired);

BENCHMARK_MAIN();
