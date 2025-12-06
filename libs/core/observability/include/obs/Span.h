#pragma once
// =============================================================================
// obs/Span.h - RAII Span for distributed tracing
// =============================================================================

#include "Context.h"
#include <string_view>
#include <memory>
#include <type_traits>

namespace obs {

/// RAII Span - automatically ends when destroyed.
/// Uses NVI (Non-Virtual Interface) pattern for bool to prevent
/// string literals from matching bool overload.
class Span {
public:
    Span() = default;
    virtual ~Span() = default;
    
    // Move only (RAII semantics)
    Span(Span&&) noexcept = default;
    Span& operator=(Span&&) noexcept = default;
    Span(const Span&) = delete;
    Span& operator=(const Span&) = delete;
    
    /// Set string attribute
    virtual Span& attr(std::string_view key, std::string_view value) = 0;
    
    /// Set integer attribute
    virtual Span& attr(std::string_view key, int64_t value) = 0;
    
    /// Set floating point attribute
    virtual Span& attr(std::string_view key, double value) = 0;
    
    /// Set boolean attribute (SFINAE prevents string literals from matching)
    template<typename T, typename = std::enable_if_t<std::is_same_v<T, bool>>>
    Span& attr(std::string_view key, T value) {
        return do_attr_bool(key, value);
    }
    
    /// Mark span as error with message
    virtual Span& set_error(std::string_view message) = 0;
    
    /// Mark span as OK (default)
    virtual Span& set_ok() = 0;
    
    /// Add an event (point-in-time occurrence within span)
    virtual Span& event(std::string_view name) = 0;
    
    /// Get context for propagation to child jobs/spans
    virtual Context context() const = 0;
    
    /// Check if span is recording (sampling decision)
    virtual bool is_recording() const = 0;

private:
    /// NVI: derived classes override this for bool attributes
    virtual Span& do_attr_bool(std::string_view key, bool value) = 0;
};

// Forward declaration - actual function uses backend
std::unique_ptr<Span> span(std::string_view name, const Context& ctx);
std::unique_ptr<Span> span(std::string_view name);

} // namespace obs
