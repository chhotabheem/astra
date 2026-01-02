#pragma once

#include "IMessageHandler.h"
#include "IQueue.h"
#include "Message.h"
#include "execution.pb.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace astra::execution {

/**
 * @brief StickyQueue - per-worker queues with session affinity.
 *
 * Messages with same session_id go to same worker (sticky routing).
 * Ensures ordering within a session.
 * Uses modulo: worker_index = session_id % num_workers
 */
class StickyQueue : public IQueue {
public:
  /**
   * @brief Construct from proto config.
   */
  StickyQueue(const ::execution::StickyQueueConfig &config,
              IMessageHandler &handler);

  /**
   * @brief Construct a sticky queue.
   * @param num_workers Number of worker threads
   * @param handler Handler that receives delivered messages
   */
  StickyQueue(size_t num_workers, IMessageHandler &handler);

  ~StickyQueue();

  // Non-copyable, non-movable
  StickyQueue(const StickyQueue &) = delete;
  StickyQueue &operator=(const StickyQueue &) = delete;
  StickyQueue(StickyQueue &&) = delete;
  StickyQueue &operator=(StickyQueue &&) = delete;

  /**
   * @brief Start worker threads.
   */
  void start();

  /**
   * @brief Stop worker threads (processes pending messages first).
   */
  void stop();

  /**
   * @brief Submit a message for delivery.
   *
   * Message is routed to worker based on session_id % num_workers.
   *
   * @param msg Message to deliver
   * @return true if submitted successfully, false if pool is stopped
   */
  bool submit(Message msg) override;

  /**
   * @brief Get number of worker threads.
   */
  [[nodiscard]] size_t worker_count() const {
    return m_num_workers;
  }

private:
  struct Worker {
    std::thread thread;
    std::deque<Message> queue;
    std::mutex mutex;
    std::condition_variable cv;
  };

  void worker_loop(size_t index);
  size_t select_worker(uint64_t session_id) const;

  size_t m_num_workers;
  std::vector<std::unique_ptr<Worker>> m_workers;
  IMessageHandler &m_handler;
  std::atomic<bool> m_running{false};
};

} // namespace astra::execution
