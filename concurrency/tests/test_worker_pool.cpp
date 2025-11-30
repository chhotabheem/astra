#include "WorkerPool.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>

using namespace astra::concurrency;

void test_basic_lifecycle() {
    std::cout << "Running test_basic_lifecycle..." << std::endl;
    WorkerPool pool(2);
    pool.start();
    pool.stop();
    std::cout << "PASSED" << std::endl;
}

void test_submit_jobs() {
    std::cout << "Running test_submit_jobs..." << std::endl;
    WorkerPool pool(4);
    pool.start();

    for (int i = 0; i < 100; ++i) {
        // Use std::any to store an integer
        Job job{JobType::HTTP_REQUEST, (uint64_t)i, i};
        bool submitted = pool.submit(job);
        assert(submitted && "Job submission failed");
        
        // Verify we can cast it back (simulated)
        try {
            int value = std::any_cast<int>(job.payload);
            assert(value == i);
        } catch (const std::bad_any_cast& e) {
            assert(false && "Failed to cast payload back to int");
        }
    }

    pool.stop();
    std::cout << "PASSED" << std::endl;
}

int main() {
    test_basic_lifecycle();
    test_submit_jobs();
    return 0;
}
