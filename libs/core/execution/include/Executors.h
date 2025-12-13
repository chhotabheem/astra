#pragma once

#include "IExecutor.h"
#include "SharedQueue.h"
#include "ISharedQueue.h"
#include <functional>
#include <memory>

namespace astra::execution {

class SharedQueueExecutor : public IExecutor {
public:
    // Takes exclusive ownership of the queue
    explicit SharedQueueExecutor(std::unique_ptr<ISharedQueue> queue)
        : m_queue(std::move(queue)) {}

    // Factory for convenience - creates and starts a queue
    static std::unique_ptr<SharedQueueExecutor> create(size_t num_workers = 4) {
        auto queue = std::make_unique<SharedQueue>(num_workers);
        queue->start();
        return std::make_unique<SharedQueueExecutor>(std::move(queue));
    }

    ~SharedQueueExecutor() override {
        if (m_queue) {
            m_queue->stop();
        }
    }

    void submit(std::function<void()> task) override {
        Message msg{0, obs::Context{}, std::move(task)};
        m_queue->submit(std::move(msg));
    }

private:
    std::unique_ptr<ISharedQueue> m_queue;
};

class InlineExecutor : public IExecutor {
public:
    void submit(std::function<void()> task) override {
        task();
    }
};

} // namespace astra::execution
