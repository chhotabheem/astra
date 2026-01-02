#include "StickyQueue.h"

#include <optional>

namespace astra::execution {

StickyQueue::StickyQueue(const ::execution::StickyQueueConfig &config,
                         IMessageHandler &handler)
    : StickyQueue(config.num_workers(), handler) {
}

StickyQueue::StickyQueue(size_t num_workers, IMessageHandler &handler)
    : m_num_workers(num_workers), m_handler(handler) {
  m_workers.reserve(m_num_workers);
  for (size_t i = 0; i < m_num_workers; ++i) {
    m_workers.push_back(std::make_unique<Worker>());
  }
}

StickyQueue::~StickyQueue() {
  if (m_running.load(std::memory_order_acquire)) {
    stop();
  }
}

void StickyQueue::start() {
  if (m_running.load(std::memory_order_acquire)) {
    return; // Already running
  }

  m_running.store(true, std::memory_order_release);

  for (size_t i = 0; i < m_num_workers; ++i) {
    m_workers[i]->thread = std::thread(&StickyQueue::worker_loop, this, i);
  }
}

void StickyQueue::stop() {
  if (!m_running.load(std::memory_order_acquire)) {
    return; // Already stopped
  }

  m_running.store(false, std::memory_order_release);

  // Wake up all workers
  for (auto &worker : m_workers) {
    std::lock_guard<std::mutex> lock(worker->mutex);
    worker->cv.notify_one();
  }

  // Join all threads
  for (auto &worker : m_workers) {
    if (worker->thread.joinable()) {
      worker->thread.join();
    }
  }
}

bool StickyQueue::submit(Message msg) {
  if (!m_running.load(std::memory_order_acquire)) {
    return false;
  }

  size_t worker_idx = select_worker(msg.session_id);
  Worker &worker = *m_workers[worker_idx];

  {
    std::lock_guard<std::mutex> lock(worker.mutex);
    worker.queue.push_back(std::move(msg));
  }
  worker.cv.notify_one();

  return true;
}

void StickyQueue::worker_loop(size_t index) {
  Worker &worker = *m_workers[index];

  while (true) {
    std::optional<Message> msg;

    {
      std::unique_lock<std::mutex> lock(worker.mutex);
      worker.cv.wait(lock, [&] {
        return !worker.queue.empty() ||
               !m_running.load(std::memory_order_acquire);
      });

      // Process remaining messages even when stopping
      if (!worker.queue.empty()) {
        msg = std::move(worker.queue.front());
        worker.queue.pop_front();
      } else if (!m_running.load(std::memory_order_acquire)) {
        // No more messages and stopping
        break;
      }
    }

    if (msg) {
      try {
        m_handler.handle(*msg);
      } catch (...) {
        // Let decorator handle exceptions
      }
    }
  }
}

size_t StickyQueue::select_worker(uint64_t session_id) const {
  return session_id % m_num_workers;
}

} // namespace astra::execution
