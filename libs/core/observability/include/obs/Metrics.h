#pragma once

#include "Context.h"
#include <string_view>
#include <memory>

namespace obs {

class Counter {
public:
    virtual ~Counter() = default;
    virtual void inc() = 0;
    virtual void inc(int64_t value) = 0;
    virtual void inc(int64_t value, const Context& exemplar) = 0;
};

class Gauge {
public:
    virtual ~Gauge() = default;
    virtual void set(double value) = 0;
    virtual void inc() = 0;
    virtual void dec() = 0;
    virtual void inc(double value) = 0;
    virtual void dec(double value) = 0;
};

class Histogram {
public:
    virtual ~Histogram() = default;
    virtual void record(double value) = 0;
    virtual void record(double value, const Context& exemplar) = 0;
};

Counter& counter(std::string_view name, std::string_view description = "");
Gauge& gauge(std::string_view name, std::string_view description = "");
Histogram& histogram(std::string_view name, std::string_view description = "");

} // namespace obs
