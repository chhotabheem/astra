#include "resilience/impl/AtomicLoadShedder.h"

namespace astra::resilience {

AtomicLoadShedder::AtomicLoadShedder(LoadShedderPolicy policy)
    : m_max_concurrent(policy.max_concurrent), m_name(std::move(policy.name)) {
}

std::optional<LoadShedderGuard> AtomicLoadShedder::try_acquire() {
  size_t current = m_in_flight.load(std::memory_order_relaxed);

  while (true) {
    size_t max = m_max_concurrent.load(std::memory_order_relaxed);

    if (current >= max) {
      return std::nullopt;
    }

    if (m_in_flight.compare_exchange_weak(current, current + 1,
                                          std::memory_order_acquire,
                                          std::memory_order_relaxed)) {
      return LoadShedderGuard::create([this]() {
        release();
      });
    }
  }
}

void AtomicLoadShedder::release() {
  m_in_flight.fetch_sub(1, std::memory_order_release);
}

void AtomicLoadShedder::update_policy(const LoadShedderPolicy &policy) {
  m_max_concurrent.store(policy.max_concurrent, std::memory_order_relaxed);
}

size_t AtomicLoadShedder::current_count() const {
  return m_in_flight.load(std::memory_order_relaxed);
}

size_t AtomicLoadShedder::max_concurrent() const {
  return m_max_concurrent.load(std::memory_order_relaxed);
}

} // namespace astra::resilience
