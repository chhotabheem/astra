#pragma once

#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/summary.h>
#include <memory>
#include <string>
#include <map>
#include <mutex>

namespace prometheus_client {

class PrometheusManager {
public:
    static PrometheusManager& GetInstance();

    // Delete copy constructor and assignment operator
    PrometheusManager(const PrometheusManager&) = delete;
    PrometheusManager& operator=(const PrometheusManager&) = delete;

    [[nodiscard]] std::shared_ptr<prometheus::Registry> GetRegistry() const;

    // Helper to create a family of counters
    [[nodiscard]] prometheus::Family<prometheus::Counter>& GetCounterFamily(const std::string& name, const std::string& help);

    // Helper to create a family of gauges
    [[nodiscard]] prometheus::Family<prometheus::Gauge>& GetGaugeFamily(const std::string& name, const std::string& help);

    // Helper to create a family of histograms
    [[nodiscard]] prometheus::Family<prometheus::Histogram>& GetHistogramFamily(const std::string& name, const std::string& help);

private:
    PrometheusManager();
    ~PrometheusManager() = default;

    std::shared_ptr<prometheus::Registry> registry_;
    
    // Cache families to avoid recreation (optional, but good for performance)
    std::map<std::string, prometheus::Family<prometheus::Counter>*> counter_families_;
    std::map<std::string, prometheus::Family<prometheus::Gauge>*> gauge_families_;
    std::map<std::string, prometheus::Family<prometheus::Histogram>*> histogram_families_;
    
    mutable std::mutex mutex_;
};

} // namespace prometheus_client
