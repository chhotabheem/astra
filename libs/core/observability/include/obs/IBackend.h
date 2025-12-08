#pragma once

#include "Context.h"
#include <string_view>
#include <memory>

namespace obs {

class Span;
class Counter;
class Gauge;
class Histogram;

enum class Level {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

class IBackend {
public:
    virtual ~IBackend() = default;
    virtual void shutdown() = 0;
    virtual std::unique_ptr<Span> create_span(std::string_view name, const Context& ctx) = 0;
    virtual std::unique_ptr<Span> create_root_span(std::string_view name) = 0;
    virtual void log(Level level, std::string_view message, const Context& ctx) = 0;
    virtual std::shared_ptr<Counter> get_counter(std::string_view name, std::string_view desc) = 0;
    virtual std::shared_ptr<Gauge> get_gauge(std::string_view name, std::string_view desc) = 0;
    virtual std::shared_ptr<Histogram> get_histogram(std::string_view name, std::string_view desc) = 0;
};

void set_backend(std::unique_ptr<IBackend> backend);
void shutdown();

} // namespace obs
