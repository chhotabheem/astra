#pragma once
#include <Metrics.h>
#include <string_view>
#include <unordered_map>
#include <string>

namespace obs {

// MetricsRegistry - Convenience class for organizing metrics in a single member variable
// Provides fluent API for registration and short-key lookup
class MetricsRegistry {
public:
    // Fluent registration API (returns *this for chaining)
    MetricsRegistry& counter(
        std::string_view key, 
        std::string_view full_name, 
        Unit unit = Unit::Dimensionless);
    
    MetricsRegistry& histogram(
        std::string_view key, 
        std::string_view full_name, 
        Unit unit = Unit::Milliseconds);
    
    MetricsRegistry& duration_histogram(
        std::string_view key,
        std::string_view full_name);
    
    MetricsRegistry& gauge(
        std::string_view key, 
        std::string_view full_name, 
        Unit unit = Unit::Dimensionless);
    
    // Lookup by short key
    Counter counter(std::string_view key) const;
    Histogram histogram(std::string_view key) const;
    DurationHistogram duration_histogram(std::string_view key) const;
    Gauge gauge(std::string_view key) const;
    
private:
    std::unordered_map<std::string, Counter> m_counters;
    std::unordered_map<std::string, Histogram> m_histograms;
    std::unordered_map<std::string, DurationHistogram> m_duration_histograms;
    std::unordered_map<std::string, Gauge> m_gauges;
};

} // namespace obs
