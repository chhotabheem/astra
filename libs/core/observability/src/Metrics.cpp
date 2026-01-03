#include "ProviderImpl.h"

#include <Metrics.h>
#include <Provider.h>
#include <opentelemetry/common/key_value_iterable_view.h>
#include <opentelemetry/context/context.h>
#include <unordered_map>
#include <vector>

namespace astra::observability {

namespace {
// Maximum cache size for ad-hoc metrics (Fix #3)
constexpr size_t MAX_CACHE_SIZE = 100;

// Convert Attributes to OTel format
auto to_otel_attrs(Attributes attrs) {
  std::vector<std::pair<std::string, std::string>> vec;
  vec.reserve(attrs.size());
  for (const auto &[key, val] : attrs) {
    vec.emplace_back(key, val);
  }
  return vec;
}
} // namespace

// =============================================================================
// Counter Implementation
// =============================================================================

void Counter::inc(uint64_t delta) const noexcept {
  auto &inst = Provider::instance().impl().get_counter(m_id);
  if (inst) {
    inst->Add(delta);
  }
}

void Counter::inc(uint64_t delta, Attributes attrs) const noexcept {
  auto &inst = Provider::instance().impl().get_counter(m_id);
  if (inst) {
    auto otel_attrs = to_otel_attrs(attrs);
    inst->Add(delta,
              opentelemetry::common::KeyValueIterableView<decltype(otel_attrs)>(
                  otel_attrs));
  }
}

Counter register_counter(const std::string &name, Unit unit) {
  uint32_t id = Provider::instance().impl().register_counter(name, unit);
  return Counter{id};
}

Counter counter(const std::string &name, Unit unit) {
  // Thread-local cache for ad-hoc counters
  thread_local std::unordered_map<std::string, Counter> cache;

  // Fix #3: Prevent unbounded growth
  if (cache.size() >= MAX_CACHE_SIZE) {
    cache.clear(); // Simple strategy: clear entire cache
  }

  auto it = cache.find(name);
  if (it != cache.end()) {
    return it->second;
  }

  auto c = register_counter(name, unit);
  cache[name] = c;
  return c;
}

// =============================================================================
// Histogram Implementation
// =============================================================================

void Histogram::record(double value) const noexcept {
  auto &inst = Provider::instance().impl().get_histogram(m_id);
  if (inst) {
    // OTel Histogram requires a Context parameter
    inst->Record(value, opentelemetry::context::Context{});
  }
}

void Histogram::record(double value, Attributes attrs) const noexcept {
  auto &inst = Provider::instance().impl().get_histogram(m_id);
  if (inst) {
    auto otel_attrs = to_otel_attrs(attrs);
    inst->Record(
        value,
        opentelemetry::common::KeyValueIterableView<decltype(otel_attrs)>(
            otel_attrs),
        opentelemetry::context::Context{});
  }
}

Histogram register_histogram(const std::string &name, Unit unit) {
  uint32_t id = Provider::instance().impl().register_histogram(name, unit);
  return Histogram{id};
}

Histogram histogram(const std::string &name, Unit unit) {
  thread_local std::unordered_map<std::string, Histogram> cache;

  // Fix #3: Prevent unbounded growth
  if (cache.size() >= MAX_CACHE_SIZE) {
    cache.clear();
  }

  auto it = cache.find(name);
  if (it != cache.end()) {
    return it->second;
  }

  auto h = register_histogram(name, unit);
  cache[name] = h;
  return h;
}

// =============================================================================
// DurationHistogram Implementation
// =============================================================================

void DurationHistogram::record_ms(double ms) const noexcept {
  auto &inst = Provider::instance().impl().get_histogram(m_id);
  if (inst) {
    inst->Record(ms, opentelemetry::context::Context{});
  }
}

void DurationHistogram::record_ms(double ms, Attributes attrs) const noexcept {
  auto &inst = Provider::instance().impl().get_histogram(m_id);
  if (inst) {
    auto otel_attrs = to_otel_attrs(attrs);
    inst->Record(
        ms,
        opentelemetry::common::KeyValueIterableView<decltype(otel_attrs)>(
            otel_attrs),
        opentelemetry::context::Context{});
  }
}

DurationHistogram register_duration_histogram(const std::string &name) {
  uint32_t id =
      Provider::instance().impl().register_histogram(name, Unit::Milliseconds);
  return DurationHistogram{id};
}

// =============================================================================
// Gauge Implementation
// =============================================================================

void Gauge::set(int64_t value) const noexcept {
  auto &inst = Provider::instance().impl().get_gauge(m_id);
  if (inst) {
    auto &impl = Provider::instance().impl();
    auto &current = impl.get_gauge_value(m_id);

    // Compute delta
    int64_t old_value = current.load(std::memory_order_relaxed);
    int64_t delta = value - old_value;

    // Update OTel with delta
    inst->Add(delta);

    // Store new value
    current.store(value, std::memory_order_relaxed);
  }
}

void Gauge::set(int64_t value, Attributes attrs) const noexcept {
  auto &inst = Provider::instance().impl().get_gauge(m_id);
  if (inst) {
    auto &impl = Provider::instance().impl();
    auto &current = impl.get_gauge_value(m_id);

    int64_t old_value = current.load(std::memory_order_relaxed);
    int64_t delta = value - old_value;

    auto otel_attrs = to_otel_attrs(attrs);
    inst->Add(delta,
              opentelemetry::common::KeyValueIterableView<decltype(otel_attrs)>(
                  otel_attrs));

    current.store(value, std::memory_order_relaxed);
  }
}

void Gauge::add(int64_t delta) const noexcept {
  auto &inst = Provider::instance().impl().get_gauge(m_id);
  if (inst) {
    inst->Add(delta);
  }
}

void Gauge::add(int64_t delta, Attributes attrs) const noexcept {
  auto &inst = Provider::instance().impl().get_gauge(m_id);
  if (inst) {
    auto otel_attrs = to_otel_attrs(attrs);
    inst->Add(delta,
              opentelemetry::common::KeyValueIterableView<decltype(otel_attrs)>(
                  otel_attrs));
  }
}

Gauge register_gauge(const std::string &name, Unit unit) {
  uint32_t id = Provider::instance().impl().register_gauge(name, unit);
  return Gauge{id};
}

Gauge gauge(const std::string &name, Unit unit) {
  thread_local std::unordered_map<std::string, Gauge> cache;

  // Fix #3: Prevent unbounded growth
  if (cache.size() >= MAX_CACHE_SIZE) {
    cache.clear();
  }

  auto it = cache.find(name);
  if (it != cache.end()) {
    return it->second;
  }

  auto g = register_gauge(name, unit);
  cache[name] = g;
  return g;
}

} // namespace astra::observability
