#pragma once

#include "IThreadPool.h"
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace astra::execution {

class ThreadPool : public IThreadPool {
public:
    ThreadPool(size_t num_threads, size_t max_jobs = 10000);
    ~ThreadPool() override;

    void start() override;
    void stop() override;
    bool submit(Job job) override;

private:
    void worker_loop();

    size_t m_num_threads;
    size_t m_max_jobs;
    
    std::vector<std::thread> m_threads;
    std::queue<Job> m_queue;
    
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_running{false};
};

} // namespace astra::execution
