#include "StripedThreadPool.h"
#include <obs/Context.h>
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <functional>

using namespace astra::execution;

void test_basic_lifecycle() {
    std::cout << "Running test_basic_lifecycle..." << std::endl;
    StripedThreadPool pool(2);
    pool.start();
    pool.stop();
    std::cout << "PASSED" << std::endl;
}

void test_submit_jobs() {
    std::cout << "Running test_submit_jobs..." << std::endl;
    StripedThreadPool pool(4);
    pool.start();

    for (int i = 0; i < 100; ++i) {
        Job job{JobType::TASK, (uint64_t)i, std::function<void()>([]{}), obs::Context{}};
        bool submitted = pool.submit(job);
        assert(submitted && "Job submission failed");
    }

    pool.stop();
    std::cout << "PASSED" << std::endl;
}

int main() {
    test_basic_lifecycle();
    test_submit_jobs();
    return 0;
}
