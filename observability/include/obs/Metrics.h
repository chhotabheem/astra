#pragma once
// =============================================================================
// obs/Metrics.h - Counter, Histogram for application metrics
// =============================================================================

#include "Context.h"
#include <string_view>
#include <memory>

namespace obs {

/// Counter - monotonically increasing value (virtual for mocking)
class Counter {
public:
    virtual ~Counter() = default;
    
    /// Increment by 1
    virtual void inc() = 0;
    
    /// Increment by value
    virtual void inc(int64_t value) = 0;
    
    /// Increment with exemplar context (for trace correlation in metrics)
    virtual void inc(int64_t value, const Context& exemplar) = 0;
};

/// Histogram - distribution of values (virtual for mocking)
class Histogram {
public:
    virtual ~Histogram() = default;
    
    /// Record a value
    virtual void record(double value) = 0;
    
    /// Record with exemplar context
    virtual void record(double value, const Context& exemplar) = 0;
};

/// Get or create a counter (cached internally)
Counter& counter(std::string_view name, std::string_view description = "");

/// Get or create a histogram (cached internally)
Histogram& histogram(std::string_view name, std::string_view description = "");

} // namespace obs
