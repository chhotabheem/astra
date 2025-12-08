#pragma once

#include "IExecutor.h"
#include "ThreadPool.h"
#include "Job.h"
#include <functional>
#include <memory>

namespace astra::execution {

class ThreadPoolExecutor : public IExecutor {
public:
    // Takes exclusive ownership of the pool
    explicit ThreadPoolExecutor(std::unique_ptr<IThreadPool> pool)
        : m_pool(std::move(pool)) {}

    // Factory for convenience - creates and starts a pool
    static std::unique_ptr<ThreadPoolExecutor> create(size_t num_threads = 4) {
        auto pool = std::make_unique<ThreadPool>(num_threads);
        pool->start();
        return std::make_unique<ThreadPoolExecutor>(std::move(pool));
    }

    ~ThreadPoolExecutor() override {
        if (m_pool) {
            m_pool->stop();
        }
    }

    void submit(std::function<void()> task) override {
        Job job{JobType::TASK, 0, std::move(task), obs::Context{}};
        m_pool->submit(std::move(job));
    }

private:
    std::unique_ptr<IThreadPool> m_pool;
};

class InlineExecutor : public IExecutor {
public:
    void submit(std::function<void()> task) override {
        task();
    }
};

} // namespace astra::execution
