#pragma once

#include "Message.h"

#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>

namespace astra::execution {

class MessageQueue {
public:
  MessageQueue() = default;
  ~MessageQueue() = default;

  MessageQueue(const MessageQueue &) = delete;
  MessageQueue &operator=(const MessageQueue &) = delete;

  void push(Message msg);
  std::optional<Message> pop();
  void close();

private:
  std::deque<Message> m_queue;
  std::mutex m_mutex;
  std::condition_variable m_cv;
  bool m_closed{false};
};

} // namespace astra::execution
