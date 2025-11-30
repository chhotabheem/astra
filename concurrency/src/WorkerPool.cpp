#include "WorkerPool.h"
#include <iostream>

namespace astra::concurrency {

WorkerPool::WorkerPool(size_t num_threads) : m_num_threads(num_threads) {
    m_workers.reserve(m_num_threads);
    for (size_t i = 0; i < m_num_threads; ++i) {
        m_workers.push_back(std::make_unique<Worker>());
    }
}

WorkerPool::~WorkerPool() {
    stop();
}

void WorkerPool::start() {
    if (m_running) return;
    m_running = true;

    for (size_t i = 0; i < m_num_threads; ++i) {
        m_workers[i]->thread = std::thread(&WorkerPool::worker_loop, this, i);
    }
}

void WorkerPool::stop() {
    if (!m_running) return;
    m_running = false;

    // Wake up all workers so they can exit
    for (auto& worker : m_workers) {
        {
            std::lock_guard<std::mutex> lock(worker->mutex);
        } // Flush lock
        worker->cv.notify_all();
    }

    for (auto& worker : m_workers) {
        if (worker->thread.joinable()) {
            worker->thread.join();
        }
    }
}

bool WorkerPool::submit(Job job) {
    if (!m_running) return false;

    // Sharding Logic: Route to specific worker based on session_id
    size_t worker_index = job.session_id % m_num_threads;
    auto& worker = m_workers[worker_index];

    {
        std::lock_guard<std::mutex> lock(worker->mutex);
        worker->queue.push_back(job);
    }
    worker->cv.notify_one();
    return true;
}

void WorkerPool::worker_loop(size_t index) {
    auto& worker = m_workers[index];

    while (m_running) {
        Job job{JobType::SHUTDOWN, 0, {}};
        {
            std::unique_lock<std::mutex> lock(worker->mutex);
            worker->cv.wait(lock, [&] { 
                return !worker->queue.empty() || !m_running; 
            });

            if (!m_running && worker->queue.empty()) {
                return;
            }

            if (!worker->queue.empty()) {
                job = worker->queue.front();
                worker->queue.pop_front();
            }
        }

        // Process the job
        // In a real implementation, we would dispatch based on job.type
        // For now, we just acknowledge it.
        // TODO: Implement Job Dispatcher (Visitor or Switch)
        
        if (job.type == JobType::SHUTDOWN) continue;

        // Example processing placeholder
        // std::cout << "Worker " << index << " processing job type " << (int)job.type << " for session " << job.session_id << std::endl;
    }
}

} // namespace astra::concurrency
