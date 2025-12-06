#pragma once

#include "IExecutor.h"
#include <thread>
#include <functional>
#include <utility>
#include <iostream>

namespace astra::concurrency {

/**
 * @brief Executor that spawns a new detached thread for each task.
 * 
 * Mimics the behavior of spawning std::thread directly.
 * Note: In a real production system, this should use a thread pool.
 */
#include <cstdio>

class ThreadExecutor : public IExecutor {
public:
    void submit(std::function<void()> task) override {
        std::thread([task = std::move(task)]() {
            task();
        }).detach();
    }
};

/**
 * @brief Executor that runs tasks immediately on the calling thread.
 * 
 * Useful for deterministic unit testing.
 */
class InlineExecutor : public IExecutor {
public:
    void submit(std::function<void()> task) override {
        task();
    }
};

} // namespace astra::concurrency
