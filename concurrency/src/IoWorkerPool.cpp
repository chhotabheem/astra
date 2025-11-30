#include "IoWorkerPool.h"
#include <iostream>

namespace astra::concurrency {

IoWorkerPool::IoWorkerPool(size_t num_threads, size_t max_jobs) 
    : m_num_threads(num_threads), m_max_jobs(max_jobs) {
}

IoWorkerPool::~IoWorkerPool() {
    stop();
}

void IoWorkerPool::start() {
    if (m_running) return;
    m_running = true;

    m_threads.reserve(m_num_threads);
    for (size_t i = 0; i < m_num_threads; ++i) {
        m_threads.emplace_back(&IoWorkerPool::worker_loop, this);
    }
}

void IoWorkerPool::stop() {
    if (!m_running) return;
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_running = false;
    }
    m_cv.notify_all();

    for (auto& thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    m_threads.clear();
}

bool IoWorkerPool::submit(Job job) {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    if (!m_running) return false;

    // Backpressure Check
    if (m_queue.size() >= m_max_jobs) {
        return false; // Queue Full
    }

    m_queue.push(job);
    lock.unlock(); // Unlock before notifying to avoid wake-up latency
    m_cv.notify_one();
    
    return true;
}

void IoWorkerPool::worker_loop() {
    while (true) {
        Job job{JobType::SHUTDOWN, 0, {}};
        
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] { 
                return !m_queue.empty() || !m_running; 
            });

            if (!m_running && m_queue.empty()) {
                return;
            }

            if (!m_queue.empty()) {
                job = m_queue.front();
                m_queue.pop();
            }
        }

        if (job.type == JobType::SHUTDOWN) continue;

        // Process the job
        // In a real implementation, we would execute the callback or task
        // For now, we just acknowledge it.
        // TODO: Implement Job Dispatcher
    }
}

} // namespace astra::concurrency
