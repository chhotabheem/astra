#pragma once

#include "IWorkerPool.h"
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace astra::concurrency {

/**
 * @brief A Shared Queue Worker Pool for IO operations.
 * 
 * - Creates N threads.
 * - All threads share a SINGLE queue (Work Stealing / Load Balancing).
 * - Best for blocking IO tasks where execution time varies significantly.
 * - Implements Backpressure via bounded queue capacity.
 */
class IoWorkerPool : public IWorkerPool {
public:
    /**
     * @brief Construct a new Io Worker Pool.
     * 
     * @param num_threads Number of worker threads.
     * @param max_jobs Maximum number of pending jobs (Backpressure).
     */
    IoWorkerPool(size_t num_threads, size_t max_jobs = 10000);
    ~IoWorkerPool() override;

    void start() override;
    void stop() override;
    
    /**
     * @brief Submit a job to the shared queue.
     * 
     * @return true if submitted, false if queue is full (Backpressure).
     */
    bool submit(Job job) override;

private:
    void worker_loop();

    size_t m_num_threads;
    size_t m_max_jobs;
    
    std::vector<std::thread> m_threads;
    std::queue<Job> m_queue; // Shared Queue
    
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_running{false};
};

} // namespace astra::concurrency
