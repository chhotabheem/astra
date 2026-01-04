#include "PoolExecutor.h"

namespace astra::execution {

PoolExecutor::PoolExecutor(size_t num_threads, IMessageHandler &handler)
    : m_handler(handler), m_num_threads(num_threads) {
}

PoolExecutor::~PoolExecutor() {
  if (m_running.load()) {
    stop();
  }
}

void PoolExecutor::start() {
  if (m_running.load()) {
    return;
  }
  m_running.store(true);

  m_threads.reserve(m_num_threads);
  for (size_t i = 0; i < m_num_threads; ++i) {
    m_threads.emplace_back(&PoolExecutor::run_worker, this);
  }
}

void PoolExecutor::stop() {
  if (!m_running.load()) {
    return;
  }
  m_running.store(false);

  m_queue.close();

  for (auto &thread : m_threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }
  m_threads.clear();
}

void PoolExecutor::submit(Message msg) {
  m_queue.push(std::move(msg));
}

void PoolExecutor::run_worker() {
  while (auto msg = m_queue.pop()) {
    m_handler.handle(*msg);
  }
}

} // namespace astra::execution
