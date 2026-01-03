#pragma once
#include "observability.pb.h"

#include <Context.h>
#include <Metrics.h>
#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// OpenTelemetry headers
#include <opentelemetry/logs/logger.h>
#include <opentelemetry/logs/provider.h>
#include <opentelemetry/metrics/meter.h>
#include <opentelemetry/metrics/provider.h>
#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/nostd/unique_ptr.h>
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/span_context.h>
#include <opentelemetry/trace/tracer.h>

namespace astra::observability {

namespace nostd = opentelemetry::nostd;
namespace metrics_api = opentelemetry::metrics;
namespace trace_api = opentelemetry::trace;
namespace logs_api = opentelemetry::logs;

// Maximum metrics per type
constexpr uint32_t MAX_COUNTERS = 256;
constexpr uint32_t MAX_HISTOGRAMS = 256;
constexpr uint32_t MAX_GAUGES = 256;

/**
 * ProviderImpl - Core observability implementation
 *
 * Design:
 * - OTel instruments are created at registration time (cold path, mutex
 * protected)
 * - OTel instruments are thread-safe for recording (hot path, no locks needed)
 * - No TLS, no lazy initialization, simple and predictable
 */
class ProviderImpl {
public:
  bool init(const ::observability::Config &config);
  bool shutdown();

  // Metric registration (called at startup, returns ID)
  // Creates OTel instrument immediately - thread safe via mutex
  uint32_t register_counter(const std::string &name, Unit unit);
  uint32_t register_histogram(const std::string &name, Unit unit);
  uint32_t register_gauge(const std::string &name, Unit unit);

  // Get OTel instrument (hot path, no locks)
  // OTel instruments are thread-safe for Add/Record operations
  nostd::unique_ptr<metrics_api::Counter<uint64_t>> &get_counter(uint32_t id);
  nostd::unique_ptr<metrics_api::Histogram<double>> &get_histogram(uint32_t id);
  nostd::unique_ptr<metrics_api::UpDownCounter<int64_t>> &
  get_gauge(uint32_t id);

  // Gauge delta computation support
  std::atomic<int64_t> &get_gauge_value(uint32_t id) {
    return m_gauge_values[id];
  }

  // Tracer access
  nostd::shared_ptr<trace_api::Tracer> get_tracer();

  // Logger access
  nostd::shared_ptr<logs_api::Logger> get_logger();

  // Active context management (thread-local stack)
  Context get_active_context();
  void push_active_span(const Context &ctx);
  void pop_active_span();

  // Context conversion helper
  trace_api::SpanContext context_to_otel(const Context &ctx);

  // Generate span ID
  uint64_t generate_span_id();

private:
  // Helper: convert Unit enum to OTel string
  std::string unit_to_string(Unit unit) const;

  // OpenTelemetry providers
  nostd::shared_ptr<metrics_api::MeterProvider> m_meter_provider;
  nostd::shared_ptr<metrics_api::Meter> m_meter;
  nostd::shared_ptr<trace_api::TracerProvider> m_tracer_provider;
  nostd::shared_ptr<trace_api::Tracer> m_tracer;
  nostd::shared_ptr<logs_api::LoggerProvider> m_logger_provider;
  nostd::shared_ptr<logs_api::Logger> m_logger;

  // Shared OTel instruments (created at registration time)
  // Thread-safe for recording operations (Add/Record)
  std::array<nostd::unique_ptr<metrics_api::Counter<uint64_t>>, MAX_COUNTERS>
      m_counters;
  std::array<nostd::unique_ptr<metrics_api::Histogram<double>>, MAX_HISTOGRAMS>
      m_histograms;
  std::array<nostd::unique_ptr<metrics_api::UpDownCounter<int64_t>>, MAX_GAUGES>
      m_gauges;

  // Gauge current values (for delta computation)
  std::array<std::atomic<int64_t>, MAX_GAUGES> m_gauge_values{};

  // Name -> ID lookup for deduplication
  std::unordered_map<std::string, uint32_t> m_counter_names;
  std::unordered_map<std::string, uint32_t> m_histogram_names;
  std::unordered_map<std::string, uint32_t> m_gauge_names;

  // Mutex for registration (cold path only)
  mutable std::mutex m_registration_mutex;
};

// Thread-local active span stack
extern thread_local std::vector<Context> active_span_stack;

} // namespace astra::observability
