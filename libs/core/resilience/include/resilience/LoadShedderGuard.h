#pragma once

#include <IScopedResource.h>
#include <functional>
#include <utility>

namespace astra::resilience {

class LoadShedderGuard : public astra::execution::IScopedResource {
public:
  using ReleaseFn = std::function<void()>;

  static LoadShedderGuard create(ReleaseFn release_fn) {
    return LoadShedderGuard(std::move(release_fn));
  }

  ~LoadShedderGuard() override {
    if (m_release_fn) {
      m_release_fn();
    }
  }

  LoadShedderGuard(LoadShedderGuard &&other) noexcept
      : m_release_fn(std::exchange(other.m_release_fn, nullptr)) {
  }

  LoadShedderGuard &operator=(LoadShedderGuard &&other) noexcept {
    if (this != &other) {
      if (m_release_fn) {
        m_release_fn();
      }
      m_release_fn = std::exchange(other.m_release_fn, nullptr);
    }
    return *this;
  }

  LoadShedderGuard(const LoadShedderGuard &) = delete;
  LoadShedderGuard &operator=(const LoadShedderGuard &) = delete;

private:
  explicit LoadShedderGuard(ReleaseFn release_fn)
      : m_release_fn(std::move(release_fn)) {
  }

  ReleaseFn m_release_fn;
};

} // namespace astra::resilience
