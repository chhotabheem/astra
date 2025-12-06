#pragma once
// =============================================================================
// obs/Context.h - Trace context that flows with every Job in SEDA
// =============================================================================

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace obs {

/// 128-bit trace ID (W3C standard)
struct TraceId {
    uint64_t high{0};
    uint64_t low{0};
    
    bool is_valid() const { return high != 0 || low != 0; }
    std::string to_hex() const;
};

/// 64-bit span ID
struct SpanId {
    uint64_t value{0};
    
    bool is_valid() const { return value != 0; }
    std::string to_hex() const;
};

/// Baggage: user-defined key-value pairs that cross service boundaries
using Baggage = std::unordered_map<std::string, std::string>;

/// The context that flows with every Job in SEDA architecture.
/// This is the core primitive for distributed tracing.
struct Context {
    TraceId trace_id;
    SpanId parent_span_id;
    uint8_t trace_flags{0};  // Bit 0 = sampled
    Baggage baggage;
    
    bool is_valid() const { return trace_id.is_valid(); }
    bool is_sampled() const { return trace_flags & 0x01; }
    
    /// Create new root context (starts a new trace)
    static Context create();
    
    /// Create child context (same trace, new parent span ID)
    Context child(SpanId new_parent) const;
    
    /// HTTP header propagation (W3C Trace Context format)
    /// Format: 00-{trace_id}-{span_id}-{flags}
    std::string to_traceparent() const;
    static Context from_traceparent(std::string_view header);
    
    /// Baggage header (W3C Baggage format)
    std::string to_baggage_header() const;
    static void parse_baggage(Context& ctx, std::string_view header);
};

} // namespace obs
