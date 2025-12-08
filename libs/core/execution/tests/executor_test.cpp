#include <gtest/gtest.h>
#include "Executors.h"
#include <atomic>
#include <chrono>
#include <thread>

using namespace astra::execution;

class ExecutorTest : public ::testing::Test {};

// ThreadPoolExecutor tests

TEST_F(ExecutorTest, ThreadPoolExecutorOwnsPool) {
    auto pool = std::make_unique<ThreadPool>(2);
    pool->start();
    
    ThreadPoolExecutor executor(std::move(pool));
    
    std::atomic<int> counter{0};
    executor.submit([&counter] { counter++; });
    
    // Give time for execution
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    EXPECT_EQ(counter.load(), 1);
}

TEST_F(ExecutorTest, ThreadPoolExecutorFactoryCreatesWorkingExecutor) {
    auto executor = ThreadPoolExecutor::create(2);
    
    std::atomic<int> counter{0};
    executor->submit([&counter] { counter++; });
    executor->submit([&counter] { counter++; });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    EXPECT_EQ(counter.load(), 2);
}

TEST_F(ExecutorTest, ThreadPoolExecutorDefaultThreadCount) {
    // Factory should use default 4 threads
    auto executor = ThreadPoolExecutor::create();
    
    std::atomic<int> counter{0};
    for (int i = 0; i < 10; ++i) {
        executor->submit([&counter] { counter++; });
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_EQ(counter.load(), 10);
}

// InlineExecutor tests

TEST_F(ExecutorTest, InlineExecutorRunsSynchronously) {
    InlineExecutor executor;
    
    int value = 0;
    executor.submit([&value] { value = 42; });
    
    // Should be executed immediately, no waiting needed
    EXPECT_EQ(value, 42);
}

TEST_F(ExecutorTest, InlineExecutorMultipleTasks) {
    InlineExecutor executor;
    
    std::vector<int> results;
    executor.submit([&results] { results.push_back(1); });
    executor.submit([&results] { results.push_back(2); });
    executor.submit([&results] { results.push_back(3); });
    
    EXPECT_EQ(results.size(), 3u);
    EXPECT_EQ(results[0], 1);
    EXPECT_EQ(results[1], 2);
    EXPECT_EQ(results[2], 3);
}

// IExecutor interface tests

TEST_F(ExecutorTest, ExecutorPolymorphism) {
    std::unique_ptr<IExecutor> executor = std::make_unique<InlineExecutor>();
    
    int value = 0;
    executor->submit([&value] { value = 100; });
    
    EXPECT_EQ(value, 100);
}
