#include "InMemoryLinkRepository.h"
#include "RandomCodeGenerator.h"
#include "ResolveLink.h"
#include "ShortenLink.h"

#include <benchmark/benchmark.h>
#include <memory>

using namespace uri_shortener::application;
using namespace uri_shortener::domain;
using namespace uri_shortener::infrastructure;

// =============================================================================
// ShortenLink Use Case Benchmarks
// =============================================================================

static void BM_ShortenLink(benchmark::State &state) {
  auto repo = std::make_shared<InMemoryLinkRepository>();
  auto generator = std::make_shared<RandomCodeGenerator>();
  ShortenLink useCase(repo, generator);

  int i = 0;
  for (auto _ : state) {
    ShortenLink::Input input;
    input.original_url = "https://example.com/page/" + std::to_string(i++);

    auto result = useCase.execute(input);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_ShortenLink);

static void BM_ShortenLinkWithExpiry(benchmark::State &state) {
  auto repo = std::make_shared<InMemoryLinkRepository>();
  auto generator = std::make_shared<RandomCodeGenerator>();
  ShortenLink useCase(repo, generator);

  int i = 0;
  for (auto _ : state) {
    ShortenLink::Input input;
    input.original_url = "https://example.com/page/" + std::to_string(i++);
    input.expires_after = std::chrono::hours(24);

    auto result = useCase.execute(input);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_ShortenLinkWithExpiry);

// =============================================================================
// ResolveLink Use Case Benchmarks
// =============================================================================

static void BM_ResolveLink(benchmark::State &state) {
  auto repo = std::make_shared<InMemoryLinkRepository>();
  auto generator = std::make_shared<RandomCodeGenerator>();

  // Pre-populate with links
  ShortenLink shorten(repo, generator);
  std::vector<std::string> codes;
  for (int i = 0; i < 1000; ++i) {
    ShortenLink::Input input;
    input.original_url = "https://example.com/page/" + std::to_string(i);
    auto result = shorten.execute(input);
    if (result.is_ok()) {
      codes.push_back(result.value().short_code);
    }
  }

  ResolveLink resolve(repo);
  int idx = 0;

  for (auto _ : state) {
    ResolveLink::Input input;
    input.short_code = codes[idx % codes.size()];
    idx++;

    auto result = resolve.execute(input);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_ResolveLink);

static void BM_ResolveLinkNotFound(benchmark::State &state) {
  auto repo = std::make_shared<InMemoryLinkRepository>();
  ResolveLink resolve(repo);

  for (auto _ : state) {
    ResolveLink::Input input;
    input.short_code = "nonexistent";

    auto result = resolve.execute(input);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_ResolveLinkNotFound);

// =============================================================================
// Repository Benchmarks
// =============================================================================

static void BM_RepositoryLookup(benchmark::State &state) {
  auto repo = std::make_shared<InMemoryLinkRepository>();
  auto generator = std::make_shared<RandomCodeGenerator>();

  // Pre-populate
  ShortenLink shorten(repo, generator);
  std::vector<std::string> codes;
  for (int i = 0; i < 10000; ++i) {
    ShortenLink::Input input;
    input.original_url = "https://example.com/page/" + std::to_string(i);
    auto result = shorten.execute(input);
    if (result.is_ok()) {
      codes.push_back(result.value().short_code);
    }
  }

  int idx = 0;
  for (auto _ : state) {
    auto code = ShortCode::create(codes[idx % codes.size()]);
    if (code.is_ok()) {
      auto result = repo->find_by_code(code.value());
      benchmark::DoNotOptimize(result);
    }
    idx++;
  }
}
BENCHMARK(BM_RepositoryLookup);

BENCHMARK_MAIN();
