#pragma once

#include "resilience/ILoadShedder.h"
#include "resilience/policy/LoadShedderPolicy.h"

#include <atomic>

namespace astra::resilience {

class AtomicLoadShedder : public ILoadShedder {
public:
  explicit AtomicLoadShedder(LoadShedderPolicy policy);

  std::optional<LoadShedderGuard> try_acquire() override;
  void update_policy(const LoadShedderPolicy &policy) override;
  [[nodiscard]] size_t current_count() const override;
  [[nodiscard]] size_t max_concurrent() const override;

private:
  void release();

  std::atomic<size_t> m_in_flight{0};
  std::atomic<size_t> m_max_concurrent;
  std::string m_name;
};

} // namespace astra::resilience
