#pragma once

#include "IBackend.h"
#include "Span.h"
#include "Metrics.h"
#include <atomic>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <chrono>

namespace obs {

class ConsoleSpan : public Span {
public:
    ConsoleSpan(std::string_view name, const Context& ctx, std::mutex& mtx)
        : m_name(name), m_ctx(ctx), m_mutex(mtx)
        , m_start(std::chrono::steady_clock::now()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cerr << "[SPAN START] " << m_name 
                  << " trace=" << m_ctx.trace_id.to_hex().substr(0, 8) << "\n";
    }
    
    ~ConsoleSpan() override {
        auto duration = std::chrono::steady_clock::now() - m_start;
        auto ms = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cerr << "[SPAN END] " << m_name << " duration=" << ms << "us\n";
    }
    
    Span& attr(std::string_view key, std::string_view value) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cerr << "[SPAN ATTR] " << m_name << " " << key << "=" << value << "\n";
        return *this;
    }
    
    Span& attr(std::string_view key, int64_t value) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cerr << "[SPAN ATTR] " << m_name << " " << key << "=" << value << "\n";
        return *this;
    }
    
    Span& attr(std::string_view key, double value) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cerr << "[SPAN ATTR] " << m_name << " " << key << "=" << value << "\n";
        return *this;
    }
    
private:
    Span& do_attr_bool(std::string_view key, bool value) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cerr << "[SPAN ATTR] " << m_name << " " << key << "=" << (value ? "true" : "false") << "\n";
        return *this;
    }
    
public:
    Span& set_error(std::string_view message) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cerr << "[SPAN ERROR] " << m_name << " " << message << "\n";
        return *this;
    }
    
    Span& set_ok() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cerr << "[SPAN OK] " << m_name << "\n";
        return *this;
    }
    
    Span& event(std::string_view name) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cerr << "[SPAN EVENT] " << m_name << " " << name << "\n";
        return *this;
    }
    
    Context context() const override { return m_ctx; }
    bool is_recording() const override { return true; }

private:
    std::string m_name;
    Context m_ctx;
    std::mutex& m_mutex;
    std::chrono::steady_clock::time_point m_start;
};

class ConsoleCounter : public Counter {
public:
    ConsoleCounter(std::string_view name, std::mutex& mtx) 
        : m_name(name), m_mutex(mtx), m_value(0) {}
    
    void inc() override {
        m_value += 1;
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cerr << "[COUNTER] " << m_name << " += 1 (total=" << m_value << ")\n";
    }
    
    void inc(int64_t delta) override {
        m_value += delta;
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cerr << "[COUNTER] " << m_name << " += " << delta << " (total=" << m_value << ")\n";
    }
    
    void inc(int64_t delta, const Context&) override { inc(delta); }
    
private:
    std::string m_name;
    std::mutex& m_mutex;
    std::atomic<int64_t> m_value;
};

class ConsoleHistogram : public Histogram {
public:
    ConsoleHistogram(std::string_view name, std::mutex& mtx) 
        : m_name(name), m_mutex(mtx) {}
    
    void record(double value) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cerr << "[HISTOGRAM] " << m_name << " = " << value << "\n";
    }
    
    void record(double value, const Context&) override { record(value); }
    
private:
    std::string m_name;
    std::mutex& m_mutex;
};

class ConsoleGauge : public Gauge {
public:
    ConsoleGauge(std::string_view name, std::mutex& mtx) 
        : m_name(name), m_mutex(mtx), m_value(0) {}
    
    void set(double value) override {
        m_value.store(static_cast<int64_t>(value));
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cerr << "[GAUGE] " << m_name << " = " << m_value.load() << "\n";
    }
    
    void inc() override { inc(1.0); }
    void dec() override { dec(1.0); }
    
    void inc(double delta) override {
        m_value.fetch_add(static_cast<int64_t>(delta));
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cerr << "[GAUGE] " << m_name << " += " << delta << " (now=" << m_value.load() << ")\n";
    }
    
    void dec(double delta) override {
        m_value.fetch_sub(static_cast<int64_t>(delta));
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cerr << "[GAUGE] " << m_name << " -= " << delta << " (now=" << m_value.load() << ")\n";
    }
    
private:
    std::string m_name;
    std::mutex& m_mutex;
    std::atomic<int64_t> m_value;
};

class ConsoleBackend : public IBackend {
public:
    ConsoleBackend() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cerr << "[OBS] ConsoleBackend initialized\n";
    }
    
    void shutdown() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cerr << "[OBS] ConsoleBackend shutdown\n";
    }
    
    std::unique_ptr<Span> create_span(std::string_view name, const Context& ctx) override {
        return std::make_unique<ConsoleSpan>(name, ctx, m_mutex);
    }
    
    std::unique_ptr<Span> create_root_span(std::string_view name) override {
        return std::make_unique<ConsoleSpan>(name, Context::create(), m_mutex);
    }
    
    void log(Level level, std::string_view message, const Context& ctx) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cerr << "[" << level_str(level) << "] " << message;
        if (ctx.is_valid()) {
            std::cerr << " trace=" << ctx.trace_id.to_hex().substr(0, 8);
        }
        std::cerr << "\n";
    }
    
    std::shared_ptr<Counter> get_counter(std::string_view name, std::string_view) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto key = std::string(name);
        if (auto it = m_counters.find(key); it != m_counters.end()) {
            return it->second;
        }
        auto counter = std::make_shared<ConsoleCounter>(name, m_mutex);
        m_counters[key] = counter;
        return counter;
    }
    
    std::shared_ptr<Histogram> get_histogram(std::string_view name, std::string_view) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto key = std::string(name);
        if (auto it = m_histograms.find(key); it != m_histograms.end()) {
            return it->second;
        }
        auto hist = std::make_shared<ConsoleHistogram>(name, m_mutex);
        m_histograms[key] = hist;
        return hist;
    }
    
    std::shared_ptr<Gauge> get_gauge(std::string_view name, std::string_view) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto key = std::string(name);
        if (auto it = m_gauges.find(key); it != m_gauges.end()) {
            return it->second;
        }
        auto gauge = std::make_shared<ConsoleGauge>(name, m_mutex);
        m_gauges[key] = gauge;
        return gauge;
    }

private:
    static const char* level_str(Level level) {
        switch (level) {
            case Level::TRACE: return "TRACE";
            case Level::DEBUG: return "DEBUG";
            case Level::INFO:  return "INFO";
            case Level::WARN:  return "WARN";
            case Level::ERROR: return "ERROR";
            case Level::FATAL: return "FATAL";
            default: return "?";
        }
    }
    
    std::mutex m_mutex;
    std::unordered_map<std::string, std::shared_ptr<ConsoleCounter>> m_counters;
    std::unordered_map<std::string, std::shared_ptr<ConsoleHistogram>> m_histograms;
    std::unordered_map<std::string, std::shared_ptr<ConsoleGauge>> m_gauges;
};

} // namespace obs
