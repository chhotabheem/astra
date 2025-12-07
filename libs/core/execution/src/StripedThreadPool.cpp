#include "StripedThreadPool.h"
#include <functional>

namespace astra::execution {

StripedThreadPool::StripedThreadPool(size_t num_threads) : m_num_threads(num_threads) {
    m_workers.reserve(m_num_threads);
    for (size_t i = 0; i < m_num_threads; ++i) {
        m_workers.push_back(std::make_unique<Worker>());
    }
}

StripedThreadPool::~StripedThreadPool() {
    stop();
}

void StripedThreadPool::start() {
    if (m_running) return;
    m_running = true;

    for (size_t i = 0; i < m_num_threads; ++i) {
        m_workers[i]->thread = std::thread(&StripedThreadPool::worker_loop, this, i);
    }
}

void StripedThreadPool::stop() {
    if (!m_running) return;
    m_running = false;

    for (auto& worker : m_workers) {
        {
            std::lock_guard<std::mutex> lock(worker->mutex);
        }
        worker->cv.notify_all();
    }

    for (auto& worker : m_workers) {
        if (worker->thread.joinable()) {
            worker->thread.join();
        }
    }
}

bool StripedThreadPool::submit(Job job) {
    if (!m_running) return false;

    size_t worker_index = job.session_id % m_num_threads;
    auto& worker = m_workers[worker_index];

    {
        std::lock_guard<std::mutex> lock(worker->mutex);
        worker->queue.push_back(job);
    }
    worker->cv.notify_one();
    return true;
}

void StripedThreadPool::worker_loop(size_t index) {
    auto& worker = m_workers[index];

    while (m_running) {
        Job job = Job::shutdown();
        {
            std::unique_lock<std::mutex> lock(worker->mutex);
            worker->cv.wait(lock, [&] { 
                return !worker->queue.empty() || !m_running; 
            });

            if (!m_running && worker->queue.empty()) return;

            if (!worker->queue.empty()) {
                job = worker->queue.front();
                worker->queue.pop_front();
            }
        }
        
        if (job.type == JobType::SHUTDOWN) continue;

        if (job.type == JobType::TASK) {
            try {
                auto task = std::any_cast<std::function<void()>>(job.payload);
                task();
            } catch (const std::bad_any_cast&) {}
        }
    }
}

} // namespace astra::execution
