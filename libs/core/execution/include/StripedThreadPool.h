#pragma once

#include "IThreadPool.h"
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace astra::execution {

class StripedThreadPool : public IThreadPool {
public:
    explicit StripedThreadPool(size_t num_threads);
    ~StripedThreadPool() override;

    void start() override;
    void stop() override;
    bool submit(Job job) override;

private:
    struct Worker {
        std::thread thread;
        std::deque<Job> queue;
        std::mutex mutex;
        std::condition_variable cv;
    };

    void worker_loop(size_t index);

    size_t m_num_threads;
    std::vector<std::unique_ptr<Worker>> m_workers;
    std::atomic<bool> m_running{false};
};

} // namespace astra::execution
