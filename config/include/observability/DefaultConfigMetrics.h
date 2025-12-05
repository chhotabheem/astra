#pragma once
#include "observability/IConfigMetrics.h"
#include <obs/Metrics.h>

namespace config {

/**
 * @brief Default config metrics implementation using unified observability library.
 * 
 * Uses obs::counter() for metrics, which is backed by OpenTelemetry.
 */
class DefaultConfigMetrics : public IConfigMetrics {
public:
    void incrementReloadSuccess() override {
        obs::counter("config_reload_success_total", "Total successful config reloads").inc();
    }
    
    void incrementReloadFailure() override {
        obs::counter("config_reload_failure_total", "Total failed config reloads").inc();
    }
};

} // namespace config
