#pragma once
// =============================================================================
// OTelBackend.h - OpenTelemetry implementation of IBackend
// =============================================================================
// This file contains OpenTelemetry types - NOT exposed to other layers.
// Only included from src/, never from include/.
// =============================================================================

#include <obs/IBackend.h>
#include <obs/Span.h>
#include <obs/Metrics.h>

// OpenTelemetry headers (internal only)
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/tracer.h>
#include <opentelemetry/logs/provider.h>
#include <opentelemetry/logs/logger.h>
#include <opentelemetry/metrics/provider.h>
#include <opentelemetry/metrics/meter.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/sdk/logs/logger_provider.h>
#include <opentelemetry/sdk/metrics/meter_provider.h>

#include <memory>
#include <string>
#include <mutex>

namespace obs {

/// Configuration for OTelBackend
struct OTelConfig {
    std::string service_name;
    std::string service_version;
    std::string environment;
};

/// OpenTelemetry implementation of IBackend
class OTelBackend : public IBackend {
public:
    explicit OTelBackend(const OTelConfig& config);
    ~OTelBackend() override;
    
    // Non-copyable
    OTelBackend(const OTelBackend&) = delete;
    OTelBackend& operator=(const OTelBackend&) = delete;
    
    void shutdown() override;
    
    std::unique_ptr<Span> create_span(std::string_view name, const Context& ctx) override;
    std::unique_ptr<Span> create_root_span(std::string_view name) override;
    
    void log(Level level, std::string_view message, const Context& ctx) override;
    
    std::shared_ptr<Counter> get_counter(std::string_view name, std::string_view desc) override;
    std::shared_ptr<Gauge> get_gauge(std::string_view name, std::string_view desc) override;
    std::shared_ptr<Histogram> get_histogram(std::string_view name, std::string_view desc) override;

private:
    OTelConfig m_config;
    
    // OTel providers (using API types, SDK is implementation detail)
    std::shared_ptr<opentelemetry::trace::TracerProvider> m_tracer_provider;
    std::shared_ptr<opentelemetry::logs::LoggerProvider> m_logger_provider;
    std::shared_ptr<opentelemetry::metrics::MeterProvider> m_meter_provider;
    
    // OTel API objects
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> m_tracer;
    opentelemetry::nostd::shared_ptr<opentelemetry::logs::Logger> m_logger;
    opentelemetry::nostd::shared_ptr<opentelemetry::metrics::Meter> m_meter;
    
    std::mutex m_mutex;
    bool m_shutdown{false};
};

} // namespace obs
