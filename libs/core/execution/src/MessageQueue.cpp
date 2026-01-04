#include "MessageQueue.h"

namespace astra::execution {

void MessageQueue::push(Message msg) {
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_closed) {
      return;
    }
    m_queue.push_back(std::move(msg));
  }
  m_cv.notify_one();
}

std::optional<Message> MessageQueue::pop() {
  std::unique_lock<std::mutex> lock(m_mutex);
  m_cv.wait(lock, [this] {
    return !m_queue.empty() || m_closed;
  });

  if (m_queue.empty()) {
    return std::nullopt;
  }

  Message msg = std::move(m_queue.front());
  m_queue.pop_front();
  return msg;
}

void MessageQueue::close() {
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_closed = true;
  }
  m_cv.notify_all();
}

} // namespace astra::execution
