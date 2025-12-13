#include "SharedQueue.h"
#include <functional>
#include <optional>

namespace astra::execution {

SharedQueue::SharedQueue(size_t num_workers, size_t max_messages) 
    : m_num_workers(num_workers), m_max_messages(max_messages) {
}

SharedQueue::~SharedQueue() {
    stop();
}

void SharedQueue::start() {
    if (m_running) return;
    m_running = true;

    m_workers.reserve(m_num_workers);
    for (size_t i = 0; i < m_num_workers; ++i) {
        m_workers.emplace_back(&SharedQueue::worker_loop, this);
    }
}

void SharedQueue::stop() {
    if (!m_running) return;
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_running = false;
    }
    m_cv.notify_all();

    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    m_workers.clear();
}

bool SharedQueue::submit(Message msg) {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    if (!m_running) return false;
    if (m_queue.size() >= m_max_messages) return false;

    m_queue.push(std::move(msg));
    lock.unlock();
    m_cv.notify_one();
    
    return true;
}

void SharedQueue::worker_loop() {
    while (true) {
        std::optional<Message> msg;
        
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] { 
                return !m_queue.empty() || !m_running; 
            });

            if (!m_running && m_queue.empty()) return;

            if (!m_queue.empty()) {
                msg = std::move(m_queue.front());
                m_queue.pop();
            }
        }

        if (!msg) continue;

        try {
            auto task = std::any_cast<std::function<void()>>(msg->payload);
            task();
        } catch (const std::bad_any_cast&) {}
    }
}

} // namespace astra::execution
