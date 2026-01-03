#include "ProviderImpl.h"

#include <Log.h>
#include <Provider.h>
#include <opentelemetry/logs/logger.h>
#include <opentelemetry/logs/provider.h>
#include <vector>

namespace astra::observability {

namespace logs_api = opentelemetry::logs;
namespace nostd = opentelemetry::nostd;

// Thread-local scoped attributes stack
thread_local std::vector<std::map<std::string, std::string>>
    scoped_attributes_stack;

// Core logging function
void log(Level level, const std::string &message, Attributes attrs) {
  auto &provider = Provider::instance();
  auto &impl = provider.impl();

  auto logger = impl.get_logger();
  if (!logger) {
    return;
  }

  // Build attribute vectors
  std::vector<std::pair<std::string, std::string>> stored_attrs;
  stored_attrs.reserve(attrs.size());
  for (const auto &attr : attrs) {
    stored_attrs.emplace_back(attr.first, attr.second);
  }

  // Store trace correlation strings
  std::string trace_id_hex;
  std::string span_id_hex;
  auto active_ctx = impl.get_active_context();
  if (active_ctx.is_valid()) {
    trace_id_hex = active_ctx.trace_id.to_hex();
    span_id_hex = active_ctx.span_id.to_hex();
  }

  // Build final attributes map with all strings safely stored
  std::vector<std::pair<std::string, std::string>> final_attrs;

  // 1. Add user-provided attributes
  for (const auto &attr : stored_attrs) {
    final_attrs.push_back(attr);
  }

  // 2. Add scoped attributes
  for (const auto &scope_attrs : scoped_attributes_stack) {
    for (const auto &attr : scope_attrs) {
      final_attrs.push_back(attr);
    }
  }

  // 3. Add trace correlation
  if (active_ctx.is_valid()) {
    final_attrs.emplace_back("trace_id", trace_id_hex);
    final_attrs.emplace_back("span_id", span_id_hex);
    final_attrs.emplace_back("trace_flags",
                             std::to_string(active_ctx.trace_flags));
  }

  // Convert level to OTel severity
  logs_api::Severity otel_severity;
  switch (level) {
  case Level::Trace:
    otel_severity = logs_api::Severity::kTrace;
    break;
  case Level::Debug:
    otel_severity = logs_api::Severity::kDebug;
    break;
  case Level::Info:
    otel_severity = logs_api::Severity::kInfo;
    break;
  case Level::Warn:
    otel_severity = logs_api::Severity::kWarn;
    break;
  case Level::Error:
    otel_severity = logs_api::Severity::kError;
    break;
  case Level::Fatal:
    otel_severity = logs_api::Severity::kFatal;
    break;
  default:
    otel_severity = logs_api::Severity::kInfo;
  }

  // Emit log with all attributes stored in final_attrs
  logger->EmitLogRecord(otel_severity, message,
                        opentelemetry::common::MakeAttributes(final_attrs));
}

// ScopedLogAttributes implementation
ScopedLogAttributes::ScopedLogAttributes(Attributes attrs) {
  std::map<std::string, std::string> attrs_map;
  for (const auto &attr : attrs) {
    attrs_map[attr.first] = attr.second;
  }

  scoped_attributes_stack.push_back(std::move(attrs_map));
  m_stack_size = scoped_attributes_stack.size();
}

ScopedLogAttributes::~ScopedLogAttributes() {
  if (scoped_attributes_stack.size() == m_stack_size) {
    scoped_attributes_stack.pop_back();
  }
}

} // namespace astra::observability
