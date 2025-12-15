#include <benchmark/benchmark.h>
#include "Http2ClientPool.h"
#include "Http2Client.h"
#include <memory>

using namespace astra::http2;

// =============================================================================
// Client Pool Benchmarks
// =============================================================================

static void BM_PoolGet(benchmark::State& state) {
    ClientConfig config;
    config.set_host("localhost");
    config.set_port(8080);
    config.set_pool_size(4);
    
    Http2ClientPool pool(config);
    
    for (auto _ : state) {
        auto& client = pool.get();
        benchmark::DoNotOptimize(client);
    }
}
BENCHMARK(BM_PoolGet);

static void BM_PoolGetRoundRobin(benchmark::State& state) {
    ClientConfig config;
    config.set_host("localhost");
    config.set_port(8080);
    config.set_pool_size(4);
    
    Http2ClientPool pool(config);
    
    // Test round-robin distribution
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            auto& client = pool.get();
            benchmark::DoNotOptimize(client);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * 100);
}
BENCHMARK(BM_PoolGetRoundRobin);

// =============================================================================
// Client Construction Benchmarks
// =============================================================================

static void BM_ClientConstruction(benchmark::State& state) {
    for (auto _ : state) {
        ClientConfig config;
        config.set_host("localhost");
        config.set_port(8080);
        config.set_pool_size(1);
        
        Http2ClientPool pool(config);
        benchmark::DoNotOptimize(pool);
    }
}
BENCHMARK(BM_ClientConstruction);

static void BM_PoolConstruction(benchmark::State& state) {
    for (auto _ : state) {
        ClientConfig config;
        config.set_host("localhost");
        config.set_port(8080);
        config.set_pool_size(8);
        
        Http2ClientPool pool(config);
        benchmark::DoNotOptimize(pool);
    }
}
BENCHMARK(BM_PoolConstruction);

BENCHMARK_MAIN();
