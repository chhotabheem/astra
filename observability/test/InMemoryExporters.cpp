#include "InMemoryExporters.h"

namespace observability {
namespace testing {

// InMemoryLogExporter implementation
void InMemoryLogExporter::export_logs(const std::vector<LogRecord>& logs) {
    std::lock_guard lock(m_mutex);
    m_logs.insert(m_logs.end(), logs.begin(), logs.end());
}

std::vector<InMemoryLogExporter::LogRecord> InMemoryLogExporter::get_logs() const {
    std::lock_guard lock(m_mutex);
    return m_logs;
}

void InMemoryLogExporter::clear() {
    std::lock_guard lock(m_mutex);
    m_logs.clear();
}

// InMemorySpanExporter implementation
void InMemorySpanExporter::export_spans(const std::vector<Span>& spans) {
    std::lock_guard lock(m_mutex);
    m_spans.insert(m_spans.end(), spans.begin(), spans.end());
}

std::vector<InMemorySpanExporter::Span> InMemorySpanExporter::get_spans() const {
    std::lock_guard lock(m_mutex);
    return m_spans;
}

InMemorySpanExporter::Span InMemorySpanExporter::find_span(std::string_view name) const {
    std::lock_guard lock(m_mutex);
    for (const auto& span : m_spans) {
        if (span.name == name) {
            return span;
        }
    }
    return Span{};
}

void InMemorySpanExporter::clear() {
    std::lock_guard lock(m_mutex);
    m_spans.clear();
}

// InMemoryMetricsReader implementation
std::map<std::string, int64_t> InMemoryMetricsReader::collect_counters() const {
    std::lock_guard lock(m_mutex);
    return m_counters;
}

std::map<std::string, double> InMemoryMetricsReader::collect_gauges() const {
    std::lock_guard lock(m_mutex);
    return m_gauges;
}

void InMemoryMetricsReader::clear() {
    std::lock_guard lock(m_mutex);
    m_counters.clear();
    m_gauges.clear();
}

// Test initialization
void initialize_in_memory(
    std::shared_ptr<InMemoryLogExporter> /* log_exporter */,
    std::shared_ptr<InMemorySpanExporter> /* span_exporter */,
    std::shared_ptr<InMemoryMetricsReader> /* metrics_reader */
) {
    // TODO: Set up in-memory providers
}

} // namespace testing
} // namespace observability
