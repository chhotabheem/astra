#pragma once
#include <chrono>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <utility>

namespace astra::observability {

// Attributes for metrics (OpenTelemetry labels/tags)
using Attributes = std::initializer_list<std::pair<std::string, std::string>>;

// Unit enum for type-safe metric units
enum class Unit {
  Dimensionless, // "1" - default for counters
  Milliseconds,  // "ms"
  Seconds,       // "s"
  Bytes,         // "By"
  Kilobytes,     // "KiB"
  Megabytes,     // "MiB"
  Percent        // "%"
};

// Lightweight counter handle (8 bytes)
class Counter {
public:
  Counter() = default; // For std::unordered_map
  void inc(uint64_t delta = 1) const noexcept;
  void inc(uint64_t delta, Attributes attrs) const noexcept;

private:
  friend Counter register_counter(const std::string &, Unit);
  friend class MetricsRegistry;
  explicit Counter(uint32_t id) : m_id(id) {
  }

  uint32_t m_id{0};
};

// Lightweight histogram handle (8 bytes)
class Histogram {
public:
  Histogram() = default; // For std::unordered_map
  void record(double value) const noexcept;
  void record(double value, Attributes attrs) const noexcept;

private:
  friend Histogram register_histogram(const std::string &, Unit);
  friend class MetricsRegistry;
  explicit Histogram(uint32_t id) : m_id(id) {
  }

  uint32_t m_id{0};
};

// Duration-aware histogram (chrono support)
class DurationHistogram {
public:
  DurationHistogram() = default; // For std::unordered_map

  // Record without attributes
  template <typename Rep, typename Period>
  void record(std::chrono::duration<Rep, Period> duration) const noexcept {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    record_ms(static_cast<double>(ms.count()));
  }

  // Record with attributes (Fix #1)
  template <typename Rep, typename Period>
  void record(std::chrono::duration<Rep, Period> duration,
              Attributes attrs) const noexcept {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    record_ms(static_cast<double>(ms.count()), attrs);
  }

private:
  friend DurationHistogram register_duration_histogram(const std::string &);
  friend class MetricsRegistry;
  explicit DurationHistogram(uint32_t id) : m_id(id) {
  }

  void record_ms(double ms) const noexcept;
  void record_ms(double ms, Attributes attrs) const noexcept; // New overload
  uint32_t m_id{0};
};

// Lightweight gauge handle (8 bytes - POD for copyability)
class Gauge {
public:
  Gauge() = default; // For std::unordered_map
  void set(int64_t value) const noexcept;
  void set(int64_t value, Attributes attrs) const noexcept;
  void add(int64_t delta) const noexcept;
  void add(int64_t delta, Attributes attrs) const noexcept;

private:
  friend Gauge register_gauge(const std::string &, Unit);
  friend class MetricsRegistry;
  explicit Gauge(uint32_t id) : m_id(id) {
  }

  uint32_t m_id{0};
};

// Registration functions (store returned handle)
Counter register_counter(const std::string &name,
                         Unit unit = Unit::Dimensionless);
Histogram register_histogram(const std::string &name,
                             Unit unit = Unit::Milliseconds);
DurationHistogram register_duration_histogram(const std::string &name);
Gauge register_gauge(const std::string &name, Unit unit = Unit::Dimensionless);

// Ad-hoc functions (auto-registration, cached per thread)
Counter counter(const std::string &name, Unit unit = Unit::Dimensionless);
Histogram histogram(const std::string &name, Unit unit = Unit::Milliseconds);
Gauge gauge(const std::string &name, Unit unit = Unit::Dimensionless);

} // namespace astra::observability

// Backward compatibility alias
namespace obs = astra::observability;
