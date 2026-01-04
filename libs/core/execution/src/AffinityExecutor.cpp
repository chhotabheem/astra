#include "AffinityExecutor.h"

namespace astra::execution {

AffinityExecutor::AffinityExecutor(size_t num_lanes, IMessageHandler &handler)
    : m_handler(handler) {
  m_lanes.reserve(num_lanes);
  for (size_t i = 0; i < num_lanes; ++i) {
    m_lanes.push_back(std::make_unique<Lane>());
  }
}

AffinityExecutor::~AffinityExecutor() {
  if (m_running.load()) {
    stop();
  }
}

void AffinityExecutor::start() {
  if (m_running.load()) {
    return;
  }
  m_running.store(true);

  for (auto &lane_ptr : m_lanes) {
    Lane *lane = lane_ptr.get();
    lane_ptr->thread = std::thread([this, lane]() {
      while (auto msg = lane->queue.pop()) {
        m_handler.handle(*msg);
      }
    });
  }
}

void AffinityExecutor::stop() {
  if (!m_running.load()) {
    return;
  }
  m_running.store(false);

  for (auto &lane : m_lanes) {
    lane->queue.close();
  }

  for (auto &lane : m_lanes) {
    if (lane->thread.joinable()) {
      lane->thread.join();
    }
  }
}

void AffinityExecutor::submit(Message msg) {
  size_t lane_idx = msg.affinity_key % m_lanes.size();
  m_lanes[lane_idx]->queue.push(std::move(msg));
}

} // namespace astra::execution
