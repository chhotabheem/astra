#include "ThreadPool.h"
#include <obs/Context.h>
#include <gtest/gtest.h>
#include <vector>
#include <atomic>
#include <chrono>
#include <functional>

using namespace astra::execution;

class ThreadPoolTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ThreadPoolTest, StartStop) {
    ThreadPool pool(2);
    pool.start();
    pool.stop();
}

TEST_F(ThreadPoolTest, SubmitJob) {
    ThreadPool pool(2);
    pool.start();
    
    bool result = pool.submit({JobType::TASK, 1, std::function<void()>([]{}), obs::Context{}});
    EXPECT_TRUE(result);
    
    pool.stop();
}

TEST_F(ThreadPoolTest, Backpressure) {
    ThreadPool pool(0, 2);
    pool.start();
    
    EXPECT_TRUE(pool.submit({JobType::TASK, 1, std::function<void()>([]{}), obs::Context{}}));
    EXPECT_TRUE(pool.submit({JobType::TASK, 2, std::function<void()>([]{}), obs::Context{}}));
    EXPECT_FALSE(pool.submit({JobType::TASK, 3, std::function<void()>([]{}), obs::Context{}}));
    
    pool.stop();
}

TEST_F(ThreadPoolTest, SharedQueueBehavior) {
    ThreadPool pool(4);
    pool.start();
    
    for(int i=0; i<100; ++i) {
        pool.submit({JobType::TASK, (uint64_t)i, std::function<void()>([]{}), obs::Context{}});
    }
    
    pool.stop();
}
