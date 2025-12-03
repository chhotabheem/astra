#pragma once
#include "observability/IConfigMetrics.h"
#include "PrometheusManager.h"
#include <prometheus/counter.h>

namespace config {

class DefaultConfigMetrics : public IConfigMetrics {
public:
    DefaultConfigMetrics() {
        auto& success_family = prometheus_client::PrometheusManager::GetInstance()
            .GetCounterFamily("config_reload_success_total", "Total successful config reloads");
        m_success_counter = &success_family.Add({});
        
        auto& failure_family = prometheus_client::PrometheusManager::GetInstance()
            .GetCounterFamily("config_reload_failure_total", "Total failed config reloads");
        m_failure_counter = &failure_family.Add({});
    }
    
    void incrementReloadSuccess() override {
        m_success_counter->Increment();
    }
    
    void incrementReloadFailure() override {
        m_failure_counter->Increment();
    }
    
private:
    prometheus::Counter* m_success_counter{nullptr};
    prometheus::Counter* m_failure_counter{nullptr};
};

}
