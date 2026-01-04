#include <MetricsRegistry.h>

namespace astra::observability {

// =============================================================================
// Fluent Registration
// =============================================================================

MetricsRegistry &MetricsRegistry::counter(const std::string &key,
                                          const std::string &full_name,
                                          Unit unit) {
  auto c = register_counter(full_name, unit);
  m_counters.emplace(key, c);
  return *this;
}

MetricsRegistry &MetricsRegistry::histogram(const std::string &key,
                                            const std::string &full_name,
                                            Unit unit) {
  auto h = register_histogram(full_name, unit);
  m_histograms.emplace(key, h);
  return *this;
}

MetricsRegistry &
MetricsRegistry::duration_histogram(const std::string &key,
                                    const std::string &full_name) {
  auto dh = register_duration_histogram(full_name);
  m_duration_histograms.emplace(key, dh);
  return *this;
}

MetricsRegistry &MetricsRegistry::gauge(const std::string &key,
                                        const std::string &full_name,
                                        Unit unit) {
  auto g = register_gauge(full_name, unit);
  m_gauges.emplace(key, g);
  return *this;
}

// =============================================================================
// Lookup by Key
// =============================================================================

Counter MetricsRegistry::counter(const std::string &key) const {
  auto it = m_counters.find(key);
  if (it != m_counters.end()) {
    return it->second;
  }
  // Return null counter (safe to call inc() on - will be no-op)
  return Counter{0};
}

Histogram MetricsRegistry::histogram(const std::string &key) const {
  auto it = m_histograms.find(key);
  if (it != m_histograms.end()) {
    return it->second;
  }
  return Histogram{0};
}

DurationHistogram
MetricsRegistry::duration_histogram(const std::string &key) const {
  auto it = m_duration_histograms.find(key);
  if (it != m_duration_histograms.end()) {
    return it->second;
  }
  return DurationHistogram{0};
}

Gauge MetricsRegistry::gauge(const std::string &key) const {
  auto it = m_gauges.find(key);
  if (it != m_gauges.end()) {
    return it->second;
  }
  return Gauge{0};
}

} // namespace astra::observability
