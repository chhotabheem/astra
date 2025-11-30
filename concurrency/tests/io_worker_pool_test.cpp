#include "IoWorkerPool.h"
#include <gtest/gtest.h>
#include <vector>
#include <atomic>
#include <chrono>

using namespace astra::concurrency;

class IoWorkerPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use small pool for testing
    }
};

TEST_F(IoWorkerPoolTest, StartStop) {
    IoWorkerPool pool(2);
    pool.start();
    // Should not crash
    pool.stop();
}

TEST_F(IoWorkerPoolTest, SubmitJob) {
    IoWorkerPool pool(2);
    pool.start();
    
    bool result = pool.submit({JobType::HTTP_REQUEST, 1, {}});
    EXPECT_TRUE(result);
    
    pool.stop();
}

TEST_F(IoWorkerPoolTest, Backpressure) {
    // Pool with 0 threads (so no one pops) and max 2 jobs
    IoWorkerPool pool(0, 2);
    pool.start(); // Enable submission
    
    EXPECT_TRUE(pool.submit({JobType::HTTP_REQUEST, 1, {}})); // 1/2
    EXPECT_TRUE(pool.submit({JobType::HTTP_REQUEST, 2, {}})); // 2/2
    
    // This should fail (Backpressure)
    EXPECT_FALSE(pool.submit({JobType::HTTP_REQUEST, 3, {}})); // 3/2 -> Fail
    
    pool.stop();
}

TEST_F(IoWorkerPoolTest, SharedQueueBehavior) {
    // 4 threads, 100 jobs.
    // Since it's a shared queue, all jobs should be processed eventually.
    // We can't easily test "which thread took which job" without better instrumentation,
    // but we can verify stability.
    
    IoWorkerPool pool(4);
    pool.start();
    
    for(int i=0; i<100; ++i) {
        pool.submit({JobType::HTTP_REQUEST, (uint64_t)i, {}});
    }
    
    pool.stop();
}
