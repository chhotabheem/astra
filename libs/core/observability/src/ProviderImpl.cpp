#include "ProviderImpl.h"

#include <cstring>
#include <opentelemetry/exporters/ostream/log_record_exporter_factory.h>
#include <opentelemetry/exporters/ostream/metric_exporter_factory.h>
#include <opentelemetry/exporters/ostream/span_exporter_factory.h>
#include <opentelemetry/sdk/logs/logger_provider_factory.h>
#include <opentelemetry/sdk/logs/simple_log_record_processor_factory.h>
#include <opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_factory.h>
#include <opentelemetry/sdk/metrics/meter_provider_factory.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/sdk/trace/simple_processor_factory.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <random>

namespace astra::observability {

// Thread-local active span stack
thread_local std::vector<Context> active_span_stack;

// =============================================================================
// Initialization / Shutdown
// =============================================================================

bool ProviderImpl::init(const ::observability::Config &config) {
  std::lock_guard<std::mutex> lock(m_registration_mutex);

  // Already initialized check - if meter exists, skip
  if (m_meter) {
    return true;
  }

  try {
    // Create resource attributes
    auto resource_attrs = opentelemetry::sdk::resource::Resource::Create(
        {{"service.name", config.service_name()},
         {"service.version", config.service_version()},
         {"deployment.environment", config.environment()}});

    // ===== METRICS =====
    m_meter_provider = nostd::shared_ptr<metrics_api::MeterProvider>(
        new opentelemetry::sdk::metrics::MeterProvider());

    if (!m_meter_provider) {
      return false;
    }

    metrics_api::Provider::SetMeterProvider(m_meter_provider);

    m_meter = metrics_api::Provider::GetMeterProvider()->GetMeter(
        config.service_name(), config.service_version());

    if (!m_meter) {
      return false;
    }

    // ===== TRACING =====
    auto span_exporter =
        opentelemetry::exporter::trace::OStreamSpanExporterFactory::Create();
    auto span_processor =
        opentelemetry::sdk::trace::SimpleSpanProcessorFactory::Create(
            std::move(span_exporter));

    auto tracer_provider_unique =
        opentelemetry::sdk::trace::TracerProviderFactory::Create(
            std::move(span_processor), resource_attrs);
    m_tracer_provider = nostd::shared_ptr<trace_api::TracerProvider>(
        tracer_provider_unique.release());

    trace_api::Provider::SetTracerProvider(m_tracer_provider);

    m_tracer = trace_api::Provider::GetTracerProvider()->GetTracer(
        config.service_name(), config.service_version());

    if (!m_tracer) {
      return false;
    }

    // ===== LOGGING =====
    auto log_exporter = opentelemetry::exporter::logs::
        OStreamLogRecordExporterFactory::Create();
    auto log_processor =
        opentelemetry::sdk::logs::SimpleLogRecordProcessorFactory::Create(
            std::move(log_exporter));

    auto logger_provider_unique =
        opentelemetry::sdk::logs::LoggerProviderFactory::Create(
            std::move(log_processor), resource_attrs);
    m_logger_provider = nostd::shared_ptr<logs_api::LoggerProvider>(
        logger_provider_unique.release());

    logs_api::Provider::SetLoggerProvider(m_logger_provider);

    m_logger = logs_api::Provider::GetLoggerProvider()->GetLogger(
        config.service_name(), config.service_version());

    if (!m_logger) {
      return false;
    }

    return true;

  } catch (...) {
    return false;
  }
}

bool ProviderImpl::shutdown() {
  std::lock_guard<std::mutex> lock(m_registration_mutex);

  // Clear providers
  m_meter = nullptr;
  m_meter_provider = nullptr;
  m_tracer = nullptr;
  m_tracer_provider = nullptr;
  m_logger = nullptr;
  m_logger_provider = nullptr;

  // Clear instruments
  for (auto &counter : m_counters) {
    counter.reset();
  }
  for (auto &histogram : m_histograms) {
    histogram.reset();
  }
  for (auto &gauge : m_gauges) {
    gauge.reset();
  }

  // Clear registrations
  m_counter_names.clear();
  m_histogram_names.clear();
  m_gauge_names.clear();

  // Clear active span stack
  active_span_stack.clear();

  return true;
}

// =============================================================================
// Metric Registration (cold path, mutex protected)
// =============================================================================

uint32_t ProviderImpl::register_counter(const std::string &name, Unit unit) {
  std::lock_guard<std::mutex> lock(m_registration_mutex);

  if (!m_meter) {
    return 0; // Not initialized or shut down
  }

  // Check deduplication
  auto it = m_counter_names.find(name);
  if (it != m_counter_names.end()) {
    return it->second;
  }

  uint32_t id = static_cast<uint32_t>(m_counter_names.size());
  if (id >= MAX_COUNTERS) {
    return 0;
  }

  // Create OTel instrument immediately
  m_counters[id] = m_meter->CreateUInt64Counter(name, "", unit_to_string(unit));
  m_counter_names[name] = id;

  return id;
}

uint32_t ProviderImpl::register_histogram(const std::string &name, Unit unit) {
  std::lock_guard<std::mutex> lock(m_registration_mutex);

  if (!m_meter) {
    return 0;
  }

  auto it = m_histogram_names.find(name);
  if (it != m_histogram_names.end()) {
    return it->second;
  }

  uint32_t id = static_cast<uint32_t>(m_histogram_names.size());
  if (id >= MAX_HISTOGRAMS) {
    return 0;
  }

  m_histograms[id] =
      m_meter->CreateDoubleHistogram(name, "", unit_to_string(unit));
  m_histogram_names[name] = id;

  return id;
}

uint32_t ProviderImpl::register_gauge(const std::string &name, Unit unit) {
  std::lock_guard<std::mutex> lock(m_registration_mutex);

  if (!m_meter) {
    return 0;
  }

  auto it = m_gauge_names.find(name);
  if (it != m_gauge_names.end()) {
    return it->second;
  }

  uint32_t id = static_cast<uint32_t>(m_gauge_names.size());
  if (id >= MAX_GAUGES) {
    return 0;
  }

  m_gauges[id] =
      m_meter->CreateInt64UpDownCounter(name, "", unit_to_string(unit));
  m_gauge_names[name] = id;

  return id;
}

// =============================================================================
// Instrument Access (hot path, no locks - OTel instruments are thread-safe)
// =============================================================================

nostd::unique_ptr<metrics_api::Counter<uint64_t>> &
ProviderImpl::get_counter(uint32_t id) {
  static nostd::unique_ptr<metrics_api::Counter<uint64_t>> null_counter{
      nullptr};

  if (id >= MAX_COUNTERS || !m_counters[id]) {
    return null_counter;
  }

  return m_counters[id];
}

nostd::unique_ptr<metrics_api::Histogram<double>> &
ProviderImpl::get_histogram(uint32_t id) {
  static nostd::unique_ptr<metrics_api::Histogram<double>> null_histogram{
      nullptr};

  if (id >= MAX_HISTOGRAMS || !m_histograms[id]) {
    return null_histogram;
  }

  return m_histograms[id];
}

nostd::unique_ptr<metrics_api::UpDownCounter<int64_t>> &
ProviderImpl::get_gauge(uint32_t id) {
  static nostd::unique_ptr<metrics_api::UpDownCounter<int64_t>> null_gauge{
      nullptr};

  if (id >= MAX_GAUGES || !m_gauges[id]) {
    return null_gauge;
  }

  return m_gauges[id];
}

// =============================================================================
// Helpers
// =============================================================================

std::string ProviderImpl::unit_to_string(Unit unit) const {
  switch (unit) {
  case Unit::Dimensionless:
    return "1";
  case Unit::Milliseconds:
    return "ms";
  case Unit::Seconds:
    return "s";
  case Unit::Bytes:
    return "By";
  case Unit::Kilobytes:
    return "KiB";
  case Unit::Megabytes:
    return "MiB";
  case Unit::Percent:
    return "%";
  default:
    return "1";
  }
}

// =============================================================================
// Tracer and Logger Access
// =============================================================================

nostd::shared_ptr<trace_api::Tracer> ProviderImpl::get_tracer() {
  return m_tracer;
}

nostd::shared_ptr<logs_api::Logger> ProviderImpl::get_logger() {
  return m_logger;
}

// =============================================================================
// Active Context Management
// =============================================================================

Context ProviderImpl::get_active_context() {
  if (!active_span_stack.empty()) {
    return active_span_stack.back();
  }
  return Context{};
}

void ProviderImpl::push_active_span(const Context &ctx) {
  active_span_stack.push_back(ctx);
}

void ProviderImpl::pop_active_span() {
  if (!active_span_stack.empty()) {
    active_span_stack.pop_back();
  }
}

// =============================================================================
// Context Conversion
// =============================================================================

trace_api::SpanContext ProviderImpl::context_to_otel(const Context &ctx) {
  // Convert TraceId (128-bit) to OTel format
  std::array<uint8_t, 16> trace_id_bytes;
  uint64_t high_be = __builtin_bswap64(ctx.trace_id.high);
  uint64_t low_be = __builtin_bswap64(ctx.trace_id.low);
  std::memcpy(trace_id_bytes.data(), &high_be, 8);
  std::memcpy(trace_id_bytes.data() + 8, &low_be, 8);

  // Convert SpanId (64-bit) to OTel format
  std::array<uint8_t, 8> span_id_bytes;
  uint64_t span_be = __builtin_bswap64(ctx.span_id.value);
  std::memcpy(span_id_bytes.data(), &span_be, 8);

  return trace_api::SpanContext(
      trace_api::TraceId(
          nostd::span<const uint8_t, 16>(trace_id_bytes.data(), 16)),
      trace_api::SpanId(nostd::span<const uint8_t, 8>(span_id_bytes.data(), 8)),
      trace_api::TraceFlags(ctx.trace_flags), true);
}

uint64_t ProviderImpl::generate_span_id() {
  // Thread-local random engine (encapsulated within function)
  thread_local std::random_device rd;
  thread_local std::mt19937_64 gen(rd());
  return gen();
}

} // namespace astra::observability
