#pragma once

#include "IExecutor.h"
#include "IMessageHandler.h"
#include "MessageQueue.h"

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

namespace astra::execution {

class AffinityExecutor : public IExecutor {
public:
  AffinityExecutor(size_t num_lanes, IMessageHandler &handler);
  ~AffinityExecutor() override;

  AffinityExecutor(const AffinityExecutor &) = delete;
  AffinityExecutor &operator=(const AffinityExecutor &) = delete;

  void start();
  void stop();

  void submit(Message msg) override;

  [[nodiscard]] size_t lane_count() const {
    return m_lanes.size();
  }

private:
  struct Lane {
    MessageQueue queue;
    std::thread thread;
  };

  std::vector<std::unique_ptr<Lane>> m_lanes;
  IMessageHandler &m_handler;
  std::atomic<bool> m_running{false};
};

} // namespace astra::execution
