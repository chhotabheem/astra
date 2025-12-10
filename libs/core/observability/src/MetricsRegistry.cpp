#include <MetricsRegistry.h>

namespace obs {

// =============================================================================
// Fluent Registration
// =============================================================================

MetricsRegistry& MetricsRegistry::counter(
    std::string_view key, 
    std::string_view full_name, 
    Unit unit) {
    
    auto c = register_counter(full_name, unit);
    m_counters.emplace(std::string(key), c);
    return *this;
}

MetricsRegistry& MetricsRegistry::histogram(
    std::string_view key, 
    std::string_view full_name, 
    Unit unit) {
    
    auto h = register_histogram(full_name, unit);
    m_histograms.emplace(std::string(key), h);
    return *this;
}

MetricsRegistry& MetricsRegistry::duration_histogram(
    std::string_view key,
    std::string_view full_name) {
    
    auto dh = register_duration_histogram(full_name);
    m_duration_histograms.emplace(std::string(key), dh);
    return *this;
}

MetricsRegistry& MetricsRegistry::gauge(
    std::string_view key, 
    std::string_view full_name, 
    Unit unit) {
    
    auto g = register_gauge(full_name, unit);
    m_gauges.emplace(std::string(key), g);
    return *this;
}

// =============================================================================
// Lookup by Key
// =============================================================================

Counter MetricsRegistry::counter(std::string_view key) const {
    auto it = m_counters.find(std::string(key));
    if (it != m_counters.end()) {
        return it->second;
    }
    // Return null counter (safe to call inc() on - will be no-op)
    return Counter{0};
}

Histogram MetricsRegistry::histogram(std::string_view key) const {
    auto it = m_histograms.find(std::string(key));
    if (it != m_histograms.end()) {
        return it->second;
    }
    return Histogram{0};
}

DurationHistogram MetricsRegistry::duration_histogram(std::string_view key) const {
    auto it = m_duration_histograms.find(std::string(key));
    if (it != m_duration_histograms.end()) {
        return it->second;
    }
    return DurationHistogram{0};
}

Gauge MetricsRegistry::gauge(std::string_view key) const {
    auto it = m_gauges.find(std::string(key));
    if (it != m_gauges.end()) {
        return it->second;
    }
    return Gauge{0};
}

} // namespace obs
