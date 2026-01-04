#pragma once

#include "IExecutor.h"
#include "IMessageHandler.h"
#include "MessageQueue.h"

#include <atomic>
#include <thread>
#include <vector>

namespace astra::execution {

class PoolExecutor : public IExecutor {
public:
  PoolExecutor(size_t num_threads, IMessageHandler &handler);
  ~PoolExecutor() override;

  PoolExecutor(const PoolExecutor &) = delete;
  PoolExecutor &operator=(const PoolExecutor &) = delete;

  void start();
  void stop();

  void submit(Message msg) override;

  [[nodiscard]] size_t thread_count() const {
    return m_threads.size();
  }

private:
  void run_worker();

  MessageQueue m_queue;
  std::vector<std::thread> m_threads;
  IMessageHandler &m_handler;
  size_t m_num_threads;
  std::atomic<bool> m_running{false};
};

} // namespace astra::execution
