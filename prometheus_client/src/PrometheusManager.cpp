#include "PrometheusManager.h"

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

prometheus::Family<prometheus::Counter>& PrometheusManager::GetCounterFamily(std::string_view name, std::string_view help) {
    std::string name_str(name);
    std::lock_guard<std::mutex> lock(mutex_);
    if (counter_families_.find(name_str) != counter_families_.end()) {
        return *counter_families_[name_str];
    }
    auto& family = prometheus::BuildCounter()
        .Name(name_str)
        .Help(std::string(help))
        .Register(*registry_);
    counter_families_[name_str] = &family;
    return family;
}

prometheus::Family<prometheus::Gauge>& PrometheusManager::GetGaugeFamily(std::string_view name, std::string_view help) {
    std::string name_str(name);
    std::lock_guard<std::mutex> lock(mutex_);
    if (gauge_families_.find(name_str) != gauge_families_.end()) {
        return *gauge_families_[name_str];
    }
    auto& family = prometheus::BuildGauge()
        .Name(name_str)
        .Help(std::string(help))
        .Register(*registry_);
    gauge_families_[name_str] = &family;
    return family;
}

prometheus::Family<prometheus::Histogram>& PrometheusManager::GetHistogramFamily(std::string_view name, std::string_view help) {
    std::string name_str(name);
    std::lock_guard<std::mutex> lock(mutex_);
    if (histogram_families_.find(name_str) != histogram_families_.end()) {
        return *histogram_families_[name_str];
    }
    auto& family = prometheus::BuildHistogram()
        .Name(name_str)
        .Help(std::string(help))
        .Register(*registry_);
    histogram_families_[name_str] = &family;
    return family;
}

} // namespace prometheus_client
