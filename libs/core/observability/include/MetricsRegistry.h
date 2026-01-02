#pragma once
#include <Metrics.h>
#include <string>
#include <unordered_map>

namespace astra::observability {

// MetricsRegistry - Convenience class for organizing metrics in a single member
// variable Provides fluent API for registration and short-key lookup
class MetricsRegistry {
public:
  // Fluent registration API (returns *this for chaining)
  MetricsRegistry &counter(const std::string &key, const std::string &full_name,
                           Unit unit = Unit::Dimensionless);

  MetricsRegistry &histogram(const std::string &key,
                             const std::string &full_name,
                             Unit unit = Unit::Milliseconds);

  MetricsRegistry &duration_histogram(const std::string &key,
                                      const std::string &full_name);

  MetricsRegistry &gauge(const std::string &key, const std::string &full_name,
                         Unit unit = Unit::Dimensionless);

  // Lookup by short key
  Counter counter(const std::string &key) const;
  Histogram histogram(const std::string &key) const;
  DurationHistogram duration_histogram(const std::string &key) const;
  Gauge gauge(const std::string &key) const;

private:
  std::unordered_map<std::string, Counter> m_counters;
  std::unordered_map<std::string, Histogram> m_histograms;
  std::unordered_map<std::string, DurationHistogram> m_duration_histograms;
  std::unordered_map<std::string, Gauge> m_gauges;
};

} // namespace astra::observability
