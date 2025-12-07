#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <map>

namespace obs {

namespace TraceFlags {
    constexpr uint8_t NONE    = 0x00;
    constexpr uint8_t SAMPLED = 0x01;
    // Bits 1-7: Reserved by W3C for future use
}

struct TraceId {
    uint64_t high{0};
    uint64_t low{0};
    
    bool is_valid() const { return high != 0 || low != 0; }
    std::string to_hex() const;
};

struct SpanId {
    uint64_t value{0};
    
    bool is_valid() const { return value != 0; }
    std::string to_hex() const;
};

using Baggage = std::map<std::string, std::string>;

struct Context {
    TraceId trace_id;
    SpanId span_id;
    uint8_t trace_flags{TraceFlags::NONE};
    Baggage baggage;
    
    bool is_valid() const { return trace_id.is_valid(); }
    bool is_sampled() const { return trace_flags & TraceFlags::SAMPLED; }
    
    void set_sampled(bool sampled) {
        if (sampled) trace_flags |= TraceFlags::SAMPLED;
        else trace_flags &= ~TraceFlags::SAMPLED;
    }
    
    static Context create();
    Context child(SpanId new_span) const;
    
    std::string to_traceparent() const;
    static Context from_traceparent(std::string_view header);
    
    std::string to_baggage_header() const;
    static void parse_baggage(Context& ctx, std::string_view header);
};

} // namespace obs
