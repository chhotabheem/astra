#pragma once
// =============================================================================
// obs/Context.h - Trace context that flows with every Job in SEDA
// =============================================================================

#include <cstdint>
#include <string>
#include <string_view>
#include <map>  // P2: Changed from unordered_map for deterministic ordering

namespace obs {

// =============================================================================
// P3: TraceFlags - Formalized constants for trace flags (W3C spec)
// =============================================================================
namespace TraceFlags {
    constexpr uint8_t NONE    = 0x00;
    constexpr uint8_t SAMPLED = 0x01;  // Bit 0: trace is sampled/recorded
    // Bits 1-7: Reserved by W3C for future use
}

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
/// P2: Using std::map for deterministic iteration order (sorted by key)
using Baggage = std::map<std::string, std::string>;

/// The context that flows with every Job in SEDA architecture.
/// This is the core primitive for distributed tracing.
struct Context {
    TraceId trace_id;
    SpanId span_id;  // P1: Renamed from parent_span_id - this is the current span's ID
    uint8_t trace_flags{TraceFlags::NONE};  // P3: Using TraceFlags constant
    Baggage baggage;
    
    bool is_valid() const { return trace_id.is_valid(); }
    bool is_sampled() const { return trace_flags & TraceFlags::SAMPLED; }
    
    /// P3: Set sampled flag explicitly
    void set_sampled(bool sampled) {
        if (sampled) trace_flags |= TraceFlags::SAMPLED;
        else trace_flags &= ~TraceFlags::SAMPLED;
    }
    
    /// Create new root context (starts a new trace)
    static Context create();
    
    /// Create child context (same trace, new span ID)
    Context child(SpanId new_span) const;
    
    /// HTTP header propagation (W3C Trace Context format)
    /// Format: 00-{trace_id}-{span_id}-{flags}
    std::string to_traceparent() const;
    static Context from_traceparent(std::string_view header);
    
    /// Baggage header (W3C Baggage format)
    std::string to_baggage_header() const;
    static void parse_baggage(Context& ctx, std::string_view header);
};

} // namespace obs
