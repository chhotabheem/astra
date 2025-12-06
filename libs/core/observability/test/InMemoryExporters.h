#pragma once

#include "Observability.h"
#include <vector>
#include <mutex>
#include <string>

namespace observability {
namespace testing {

// In-memory log exporter for testing
class InMemoryLogExporter {
public:
    struct LogRecord {
        std::string timestamp;
        std::string severity;
        std::string body;
        std::string trace_id;
        std::string span_id;
        Attributes attributes;
    };
    
    void export_logs(const std::vector<LogRecord>& logs);
    std::vector<LogRecord> get_logs() const;
    void clear();
    
private:
    mutable std::mutex m_mutex;
    std::vector<LogRecord> m_logs;
};

// In-memory span exporter for testing
class InMemorySpanExporter {
public:
    struct Span {
        std::string name;
        std::string trace_id;
        std::string span_id;
        std::string parent_span_id;
        uint64_t start_time_ns;
        uint64_t end_time_ns;
        Attributes attributes;
        bool is_error = false;
    };
    
    void export_spans(const std::vector<Span>& spans);
    std::vector<Span> get_spans() const;
    Span find_span(std::string_view name) const;void clear();
    
private:
    mutable std::mutex m_mutex;
    std::vector<Span> m_spans;
};

// In-memory metrics reader for testing
class InMemoryMetricsReader {
public:
    std::map<std::string, int64_t> collect_counters() const;
    std::map<std::string, double> collect_gauges() const;
    void clear();
    
private:
    mutable std::mutex m_mutex;
    std::map<std::string, int64_t> m_counters;
    std::map<std::string, double> m_gauges;
};

// Initialize for testing
void initialize_in_memory(
    std::shared_ptr<InMemoryLogExporter> log_exporter = nullptr,
    std::shared_ptr<InMemorySpanExporter> span_exporter = nullptr,
    std::shared_ptr<InMemoryMetricsReader> metrics_reader = nullptr
);

} // namespace testing
} // namespace observability
