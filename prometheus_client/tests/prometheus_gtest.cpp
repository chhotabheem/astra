#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "PrometheusManager.h"
#include <thread>
#include <vector>

using namespace testing;
using namespace prometheus_client;

TEST(PrometheusManagerTest, CounterRegistration) {
    auto& manager = PrometheusManager::GetInstance();
    auto& family = manager.GetCounterFamily("test_counter_gtest", "A test counter");
    auto& counter = family.Add({{"label", "value"}});
    counter.Increment();
    
    auto registry = manager.GetRegistry();
    auto collected = registry->Collect();
    
    bool found = false;
    for (const auto& metric_family : collected) {
        if (metric_family.name == "test_counter_gtest") {
            found = true;
            ASSERT_EQ(metric_family.metric[0].counter.value, 1.0);
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST(PrometheusManagerTest, ConcurrentRegistration) {
    auto& manager = PrometheusManager::GetInstance();
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&manager, i] {
            // Register same family from multiple threads
            auto& family = manager.GetCounterFamily("concurrent_counter_gtest", "Concurrent test");
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
        if (metric_family.name == "concurrent_counter_gtest") {
            found = true;
            // Should have 10 metrics (one per thread label)
            EXPECT_EQ(metric_family.metric.size(), 10);
            break;
        }
    }
    EXPECT_TRUE(found);
}
