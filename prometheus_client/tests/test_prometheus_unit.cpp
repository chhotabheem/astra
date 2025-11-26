#include "PrometheusManager.hpp"
#include <iostream>
#include <cassert>
#include <thread>
#include <vector>

void test_counter_registration() {
    auto& manager = prometheus_client::PrometheusManager::GetInstance();
    auto& family = manager.GetCounterFamily("test_counter", "A test counter");
    auto& counter = family.Add({{"label", "value"}});
    counter.Increment();
    
    auto registry = manager.GetRegistry();
    auto collected = registry->Collect();
    
    bool found = false;
    for (const auto& metric_family : collected) {
        if (metric_family.name == "test_counter") {
            found = true;
            assert(metric_family.metric[0].counter.value == 1.0);
            break;
        }
    }
    assert(found);
    std::cout << "test_counter_registration passed" << std::endl;
}

void test_concurrent_registration() {
    auto& manager = prometheus_client::PrometheusManager::GetInstance();
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&manager, i] {
            // Register same family from multiple threads
            auto& family = manager.GetCounterFamily("concurrent_counter", "Concurrent test");
            auto& counter = family.Add({{"thread", std::to_string(i)}});
            counter.Increment();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto registry = manager.GetRegistry();
    auto collected = registry->Collect();
    
    bool found = false;
    for (const auto& metric_family : collected) {
        if (metric_family.name == "concurrent_counter") {
            found = true;
            // Should have 10 metrics (one per thread label)
            assert(metric_family.metric.size() == 10);
            break;
        }
    }
    assert(found);
    std::cout << "test_concurrent_registration passed" << std::endl;
}

int main() {
    test_counter_registration();
    test_concurrent_registration();
    std::cout << "All prometheus tests passed" << std::endl;
    return 0;
}
