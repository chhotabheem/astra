#pragma once

#include "IWorkerPool.hpp"
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace astra::concurrency {

/**
 * @brief A Sharded Worker Pool implementation.
 * 
 * - Creates N threads.
 * - Each thread has its own private queue.
 * - Jobs are routed to threads based on job.session_id % num_threads.
 * - Ensures Cache Locality and Zero Contention between workers.
 */
class WorkerPool : public IWorkerPool {
public:
    /**
     * @brief Construct a new Worker Pool.
     * 
     * @param num_threads Number of worker threads to spawn.
     */
    explicit WorkerPool(size_t num_threads);
    ~WorkerPool() override;

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

    size_t num_threads_;
    std::vector<std::unique_ptr<Worker>> workers_;
    std::atomic<bool> running_{false};
};

} // namespace astra::concurrency
