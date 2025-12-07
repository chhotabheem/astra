#pragma once

#include "IExecutor.h"
#include "ThreadPool.h"
#include "Job.h"
#include <functional>
#include <memory>

namespace astra::execution {

class ThreadPoolExecutor : public IExecutor {
public:
    explicit ThreadPoolExecutor(size_t num_threads = 4)
        : m_owned_pool(std::make_shared<ThreadPool>(num_threads)) {
        m_owned_pool->start();
    }

    explicit ThreadPoolExecutor(std::shared_ptr<ThreadPool> pool)
        : m_pool_ref(std::move(pool)) {}

    ~ThreadPoolExecutor() override {
        if (m_owned_pool) {
            m_owned_pool->stop();
        }
    }

    void submit(std::function<void()> task) override {
        Job job{JobType::TASK, 0, std::move(task), obs::Context{}};
        if (m_owned_pool) {
            m_owned_pool->submit(std::move(job));
        } else if (auto pool = m_pool_ref.lock()) {
            pool->submit(std::move(job));
        }
    }

private:
    std::shared_ptr<ThreadPool> m_owned_pool;
    std::weak_ptr<ThreadPool> m_pool_ref;
};

class InlineExecutor : public IExecutor {
public:
    void submit(std::function<void()> task) override {
        task();
    }
};

} // namespace astra::execution
