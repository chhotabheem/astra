#include "SharedQueue.h"
#include <Context.h>
#include <gtest/gtest.h>
#include <vector>
#include <atomic>
#include <chrono>
#include <functional>

using namespace astra::execution;

class SharedQueueTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(SharedQueueTest, StartStop) {
    SharedQueue queue(2);
    queue.start();
    queue.stop();
}

TEST_F(SharedQueueTest, SubmitMessage) {
    SharedQueue queue(2);
    queue.start();
    
    bool result = queue.submit({1, obs::Context{}, std::function<void()>([]{})}); 
    EXPECT_TRUE(result);
    
    queue.stop();
}

TEST_F(SharedQueueTest, Backpressure) {
    SharedQueue queue(0, 2);
    queue.start();
    
    EXPECT_TRUE(queue.submit({1, obs::Context{}, std::function<void()>([]{})}));
    EXPECT_TRUE(queue.submit({2, obs::Context{}, std::function<void()>([]{})}));
    EXPECT_FALSE(queue.submit({3, obs::Context{}, std::function<void()>([]{})}));
    
    queue.stop();
}

TEST_F(SharedQueueTest, SharedQueueBehavior) {
    SharedQueue queue(4);
    queue.start();
    
    for(int i=0; i<100; ++i) {
        queue.submit({(uint64_t)i, obs::Context{}, std::function<void()>([]{})});
    }
    
    queue.stop();
}
