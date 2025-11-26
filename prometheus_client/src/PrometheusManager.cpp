#include "PrometheusManager.hpp"

namespace prometheus_client {

PrometheusManager& PrometheusManager::GetInstance() {
    static PrometheusManager instance;
    return instance;
}

PrometheusManager::PrometheusManager() {
    registry_ = std::make_shared<prometheus::Registry>();
}

std::shared_ptr<prometheus::Registry> PrometheusManager::GetRegistry() const {
    return registry_;
}

prometheus::Family<prometheus::Counter>& PrometheusManager::GetCounterFamily(const std::string& name, const std::string& help) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (counter_families_.find(name) != counter_families_.end()) {
        return *counter_families_[name];
    }
    auto& family = prometheus::BuildCounter()
        .Name(name)
        .Help(help)
        .Register(*registry_);
    counter_families_[name] = &family;
    return family;
}

prometheus::Family<prometheus::Gauge>& PrometheusManager::GetGaugeFamily(const std::string& name, const std::string& help) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (gauge_families_.find(name) != gauge_families_.end()) {
        return *gauge_families_[name];
    }
    auto& family = prometheus::BuildGauge()
        .Name(name)
        .Help(help)
        .Register(*registry_);
    gauge_families_[name] = &family;
    return family;
}

prometheus::Family<prometheus::Histogram>& PrometheusManager::GetHistogramFamily(const std::string& name, const std::string& help) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (histogram_families_.find(name) != histogram_families_.end()) {
        return *histogram_families_[name];
    }
    auto& family = prometheus::BuildHistogram()
        .Name(name)
        .Help(help)
        .Register(*registry_);
    histogram_families_[name] = &family;
    return family;
}

} // namespace prometheus_client
