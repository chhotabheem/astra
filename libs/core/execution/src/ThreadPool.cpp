#include "ThreadPool.h"
#include <functional>
#include <optional>

namespace astra::execution {

ThreadPool::ThreadPool(size_t num_threads, size_t max_jobs) 
    : m_num_threads(num_threads), m_max_jobs(max_jobs) {
}

ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::start() {
    if (m_running) return;
    m_running = true;

    m_threads.reserve(m_num_threads);
    for (size_t i = 0; i < m_num_threads; ++i) {
        m_threads.emplace_back(&ThreadPool::worker_loop, this);
    }
}

void ThreadPool::stop() {
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

bool ThreadPool::submit(Job job) {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    if (!m_running) return false;
    if (m_queue.size() >= m_max_jobs) return false;

    m_queue.push(std::move(job));
    lock.unlock();
    m_cv.notify_one();
    
    return true;
}

void ThreadPool::worker_loop() {
    while (true) {
        std::optional<Job> job;
        
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] { 
                return !m_queue.empty() || !m_running; 
            });

            if (!m_running && m_queue.empty()) return;

            if (!m_queue.empty()) {
                job = std::move(m_queue.front());
                m_queue.pop();
            }
        }

        if (!job) continue;

        try {
            auto task = std::any_cast<std::function<void()>>(job->payload);
            task();
        } catch (const std::bad_any_cast&) {}
    }
}

} // namespace astra::execution
