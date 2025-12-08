// =============================================================================
// OTelBackend.cpp - OpenTelemetry implementation
// =============================================================================
#include "OTelBackend.h"

// OTel SDK includes
#include <opentelemetry/sdk/trace/simple_processor.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <opentelemetry/sdk/logs/simple_log_record_processor.h>
#include <opentelemetry/sdk/logs/logger_provider_factory.h>
#include <opentelemetry/sdk/metrics/meter_provider_factory.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/exporters/ostream/span_exporter.h>
#include <opentelemetry/exporters/ostream/log_record_exporter.h>
#include <opentelemetry/exporters/ostream/metric_exporter.h>
#include <opentelemetry/context/context.h>

#include <iostream>

namespace trace_api = opentelemetry::trace;
namespace logs_api = opentelemetry::logs;
namespace metrics_api = opentelemetry::metrics;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace logs_sdk = opentelemetry::sdk::logs;
namespace metrics_sdk = opentelemetry::sdk::metrics;
namespace resource = opentelemetry::sdk::resource;
namespace nostd = opentelemetry::nostd;

namespace obs {

// =============================================================================
// OTelSpan - Wraps OTel Span
// =============================================================================
class OTelSpan : public Span {
public:
    OTelSpan(nostd::shared_ptr<trace_api::Span> span, const Context& ctx)
        : m_span(std::move(span)), m_ctx(ctx) {}
    
    ~OTelSpan() override {
        if (m_span) {
            m_span->End();
        }
    }
    
    Span& attr(std::string_view key, std::string_view value) override {
        if (m_span) {
            m_span->SetAttribute(nostd::string_view(key.data(), key.size()),
                                nostd::string_view(value.data(), value.size()));
        }
        return *this;
    }
    
    Span& attr(std::string_view key, int64_t value) override {
        if (m_span) {
            m_span->SetAttribute(nostd::string_view(key.data(), key.size()), value);
        }
        return *this;
    }
    
    Span& attr(std::string_view key, double value) override {
        if (m_span) {
            m_span->SetAttribute(nostd::string_view(key.data(), key.size()), value);
        }
        return *this;
    }
    
private:
    Span& do_attr_bool(std::string_view key, bool value) override {
        if (m_span) {
            m_span->SetAttribute(nostd::string_view(key.data(), key.size()), value);
        }
        return *this;
    }
    
public:
    
    Span& set_error(std::string_view message) override {
        if (m_span) {
            m_span->SetStatus(trace_api::StatusCode::kError, 
                             std::string(message));
        }
        return *this;
    }
    
    Span& set_ok() override {
        if (m_span) {
            m_span->SetStatus(trace_api::StatusCode::kOk, "");
        }
        return *this;
    }
    
    Span& event(std::string_view name) override {
        if (m_span) {
            m_span->AddEvent(nostd::string_view(name.data(), name.size()));
        }
        return *this;
    }
    
    Context context() const override {
        // Return context with this span as parent
        Context child = m_ctx;
        if (m_span) {
            auto span_ctx = m_span->GetContext();
            // Extract span ID to use as parent for children
            child.span_id.value = 0;
            for (int i = 0; i < 8; ++i) {
                child.span_id.value = (child.span_id.value << 8) | 
                    span_ctx.span_id().Id()[i];
            }
        }
        return child;
    }
    
    bool is_recording() const override {
        return m_span && m_span->IsRecording();
    }

private:
    nostd::shared_ptr<trace_api::Span> m_span;
    Context m_ctx;
};

// =============================================================================
// OTelCounter - Wraps OTel Counter
// =============================================================================
class OTelCounter : public Counter {
public:
    explicit OTelCounter(nostd::unique_ptr<metrics_api::Counter<uint64_t>> counter)
        : m_counter(std::move(counter)) {}
    
    void inc() override { inc(1); }
    
    void inc(int64_t value) override {
        if (m_counter && value > 0) {
            m_counter->Add(static_cast<uint64_t>(value));
        }
    }
    
    void inc(int64_t value, const Context&) override {
        inc(value);
    }

private:
    nostd::unique_ptr<metrics_api::Counter<uint64_t>> m_counter;
};

// =============================================================================
// OTelHistogram - Wraps OTel Histogram
// =============================================================================
class OTelHistogram : public Histogram {
public:
    explicit OTelHistogram(nostd::unique_ptr<metrics_api::Histogram<double>> histogram)
        : m_histogram(std::move(histogram)) {}
    
    void record(double value) override {
        if (m_histogram) {
            m_histogram->Record(value, opentelemetry::context::Context{});
        }
    }
    
    void record(double value, const Context&) override {
        record(value);  // Exemplar support would be added here
    }

private:
    nostd::unique_ptr<metrics_api::Histogram<double>> m_histogram;
};

// =============================================================================
// OTelGauge - Wraps OTel UpDownCounter (closest to gauge semantics)
// =============================================================================
class OTelGauge : public Gauge {
public:
    explicit OTelGauge(nostd::unique_ptr<metrics_api::UpDownCounter<int64_t>> counter)
        : m_counter(std::move(counter)), m_value(0) {}
    
    void set(double value) override {
        // UpDownCounter doesn't have set(), so we simulate with add/subtract
        // This is not perfect but works for our use case
        int64_t int_val = static_cast<int64_t>(value);
        int64_t diff = int_val - m_value.load();
        m_value.store(int_val);
        if (m_counter && diff != 0) {
            m_counter->Add(diff);
        }
    }
    
    void inc() override { inc(1.0); }
    void dec() override { dec(1.0); }
    
    void inc(double value) override {
        int64_t int_val = static_cast<int64_t>(value);
        m_value.fetch_add(int_val);
        if (m_counter) {
            m_counter->Add(int_val);
        }
    }
    
    void dec(double value) override {
        int64_t int_val = static_cast<int64_t>(value);
        m_value.fetch_sub(int_val);
        if (m_counter) {
            m_counter->Add(-int_val);
        }
    }

private:
    nostd::unique_ptr<metrics_api::UpDownCounter<int64_t>> m_counter;
    std::atomic<int64_t> m_value;
};

// =============================================================================
// OTelBackend Implementation
// =============================================================================
OTelBackend::OTelBackend(const OTelConfig& config) : m_config(config) {
    // Create resource attributes
    auto resource_attrs = resource::Resource::Create({
        {"service.name", config.service_name},
        {"service.version", config.service_version},
        {"deployment.environment", config.environment}
    });
    
    // --- Trace Provider ---
    auto trace_exporter = std::make_unique<opentelemetry::exporter::trace::OStreamSpanExporter>();
    auto trace_processor = std::make_unique<trace_sdk::SimpleSpanProcessor>(std::move(trace_exporter));
    m_tracer_provider = std::shared_ptr<trace_api::TracerProvider>(
        new trace_sdk::TracerProvider(std::move(trace_processor), resource_attrs));
    trace_api::Provider::SetTracerProvider(m_tracer_provider);
    m_tracer = trace_api::Provider::GetTracerProvider()->GetTracer(config.service_name, config.service_version);
    
    // --- Log Provider ---
    auto log_exporter = std::make_unique<opentelemetry::exporter::logs::OStreamLogRecordExporter>();
    auto log_processor = std::make_unique<logs_sdk::SimpleLogRecordProcessor>(std::move(log_exporter));
    m_logger_provider = std::shared_ptr<logs_api::LoggerProvider>(
        new logs_sdk::LoggerProvider(std::move(log_processor), resource_attrs));
    logs_api::Provider::SetLoggerProvider(m_logger_provider);
    m_logger = logs_api::Provider::GetLoggerProvider()->GetLogger(config.service_name, "", config.service_version);
    
    // --- Meter Provider ---
    auto meter_exporter = std::make_unique<opentelemetry::exporter::metrics::OStreamMetricExporter>();
    m_meter_provider = std::shared_ptr<metrics_api::MeterProvider>(
        new metrics_sdk::MeterProvider());
    metrics_api::Provider::SetMeterProvider(m_meter_provider);
    m_meter = metrics_api::Provider::GetMeterProvider()->GetMeter(config.service_name, config.service_version);
}

OTelBackend::~OTelBackend() {
    shutdown();
}

void OTelBackend::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_shutdown) return;
    m_shutdown = true;
    
    // Clear API references
    m_tracer = nullptr;
    m_logger = nullptr;
    m_meter = nullptr;
    
    // Clear providers (shared_ptr cleanup handles the rest)
    m_tracer_provider.reset();
    m_logger_provider.reset();
    m_meter_provider.reset();
}

std::unique_ptr<Span> OTelBackend::create_span(std::string_view name, const Context& ctx) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_shutdown || !m_tracer) return nullptr;
    
    trace_api::StartSpanOptions options;
    options.kind = trace_api::SpanKind::kInternal;
    
    auto span = m_tracer->StartSpan(nostd::string_view(name.data(), name.size()), options);
    return std::make_unique<OTelSpan>(std::move(span), ctx);
}

std::unique_ptr<Span> OTelBackend::create_root_span(std::string_view name) {
    return create_span(name, Context::create());
}

void OTelBackend::log(Level level, std::string_view message, const Context& ctx) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_shutdown || !m_logger) return;
    
    auto record = m_logger->CreateLogRecord();
    if (!record) return;
    
    record->SetTimestamp(std::chrono::system_clock::now());
    record->SetBody(nostd::string_view(message.data(), message.size()));
    
    // Map Level to OTel Severity
    logs_api::Severity severity;
    switch (level) {
        case Level::TRACE: severity = logs_api::Severity::kTrace; break;
        case Level::DEBUG: severity = logs_api::Severity::kDebug; break;
        case Level::INFO:  severity = logs_api::Severity::kInfo; break;
        case Level::WARN:  severity = logs_api::Severity::kWarn; break;
        case Level::ERROR: severity = logs_api::Severity::kError; break;
        case Level::FATAL: severity = logs_api::Severity::kFatal; break;
    }
    record->SetSeverity(severity);
    
    // Add trace correlation if context valid
    if (ctx.is_valid()) {
        // Would set trace_id and span_id from ctx here
    }
    
    m_logger->EmitLogRecord(std::move(record));
}

std::shared_ptr<Counter> OTelBackend::get_counter(std::string_view name, std::string_view desc) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_shutdown || !m_meter) return nullptr;
    
    auto counter = m_meter->CreateUInt64Counter(
        std::string(name), 
        std::string(desc));
    return std::make_shared<OTelCounter>(std::move(counter));
}

std::shared_ptr<Gauge> OTelBackend::get_gauge(std::string_view name, std::string_view desc) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_shutdown || !m_meter) return nullptr;
    
    auto updown_counter = m_meter->CreateInt64UpDownCounter(
        std::string(name),
        std::string(desc));
    return std::make_shared<OTelGauge>(std::move(updown_counter));
}

std::shared_ptr<Histogram> OTelBackend::get_histogram(std::string_view name, std::string_view desc) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_shutdown || !m_meter) return nullptr;
    
    auto histogram = m_meter->CreateDoubleHistogram(
        std::string(name),
        std::string(desc));
    return std::make_shared<OTelHistogram>(std::move(histogram));
}

} // namespace obs
