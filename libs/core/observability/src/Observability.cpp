// =============================================================================
// Observability.cpp - Facade implementation
// =============================================================================
#include <obs/IBackend.h>
#include <obs/Span.h>
#include <obs/Log.h>
#include <obs/Metrics.h>
#include <obs/Context.h>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <string>

namespace obs {

namespace {
    std::mutex g_backend_mutex;
    std::unique_ptr<IBackend> g_backend;
    std::atomic<bool> g_initialized{false};
    
    // Cached counters, gauges, and histograms
    std::unordered_map<std::string, std::shared_ptr<Counter>> g_counters;
    std::unordered_map<std::string, std::shared_ptr<Gauge>> g_gauges;
    std::unordered_map<std::string, std::shared_ptr<Histogram>> g_histograms;
    
    // Null implementations for when no backend
    class NullCounter : public Counter {
    public:
        void inc() override {}
        void inc(int64_t) override {}
        void inc(int64_t, const Context&) override {}
    };
    
    class NullHistogram : public Histogram {
    public:
        void record(double) override {}
        void record(double, const Context&) override {}
    };
    
    class NullGauge : public Gauge {
    public:
        void set(double) override {}
        void inc() override {}
        void dec() override {}
        void inc(double) override {}
        void dec(double) override {}
    };
    
    static NullCounter g_null_counter;
    static NullGauge g_null_gauge;
    static NullHistogram g_null_histogram;
}

void set_backend(std::unique_ptr<IBackend> backend) {
    std::lock_guard<std::mutex> lock(g_backend_mutex);
    g_backend = std::move(backend);
    g_counters.clear();
    g_gauges.clear();
    g_histograms.clear();
    g_initialized.store(true);
}

void shutdown() {
    std::lock_guard<std::mutex> lock(g_backend_mutex);
    if (g_backend) {
        g_backend->shutdown();
        g_backend.reset();
    }
    g_counters.clear();
    g_gauges.clear();
    g_histograms.clear();
    g_initialized.store(false);
}

std::unique_ptr<Span> span(std::string_view name, const Context& ctx) {
    std::lock_guard<std::mutex> lock(g_backend_mutex);
    if (g_backend) {
        return g_backend->create_span(name, ctx);
    }
    return nullptr;
}

std::unique_ptr<Span> span(std::string_view name) {
    std::lock_guard<std::mutex> lock(g_backend_mutex);
    if (g_backend) {
        return g_backend->create_root_span(name);
    }
    return nullptr;
}

// -----------------------------------------------------------------------------
// Logging
// -----------------------------------------------------------------------------
void log(Level level, std::string_view message, const Context& ctx) {
    std::lock_guard<std::mutex> lock(g_backend_mutex);
    if (g_backend) {
        g_backend->log(level, message, ctx);
    }
}

void log(Level level, std::string_view message) {
    log(level, message, Context{});
}

// -----------------------------------------------------------------------------
// Metrics
// -----------------------------------------------------------------------------
Counter& counter(std::string_view name, std::string_view description) {
    std::lock_guard<std::mutex> lock(g_backend_mutex);
    if (!g_backend) {
        return g_null_counter;
    }
    
    std::string key(name);
    auto it = g_counters.find(key);
    if (it != g_counters.end()) {
        return *it->second;
    }
    
    auto c = g_backend->get_counter(name, description);
    if (c) {
        g_counters[key] = c;
        return *c;
    }
    return g_null_counter;
}

Histogram& histogram(std::string_view name, std::string_view description) {
    std::lock_guard<std::mutex> lock(g_backend_mutex);
    if (!g_backend) {
        return g_null_histogram;
    }
    
    std::string key(name);
    auto it = g_histograms.find(key);
    if (it != g_histograms.end()) {
        return *it->second;
    }
    
    auto h = g_backend->get_histogram(name, description);
    if (h) {
        g_histograms[key] = h;
        return *h;
    }
    return g_null_histogram;
}

Gauge& gauge(std::string_view name, std::string_view description) {
    std::lock_guard<std::mutex> lock(g_backend_mutex);
    if (!g_backend) {
        return g_null_gauge;
    }
    
    std::string key(name);
    auto it = g_gauges.find(key);
    if (it != g_gauges.end()) {
        return *it->second;
    }
    
    auto g = g_backend->get_gauge(name, description);
    if (g) {
        g_gauges[key] = g;
        return *g;
    }
    return g_null_gauge;
}

} // namespace obs
