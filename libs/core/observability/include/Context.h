#pragma once

#include <cstdint>
#include <map>
#include <string>

namespace astra::observability {

namespace TraceFlags {
constexpr uint8_t NONE = 0x00;
constexpr uint8_t SAMPLED = 0x01;
// Bits 1-7: Reserved by W3C for future use
} // namespace TraceFlags

struct TraceId {
  uint64_t high{0};
  uint64_t low{0};

  [[nodiscard]] constexpr bool is_valid() const {
    return high != 0 || low != 0;
  }
  [[nodiscard]] std::string to_hex() const;
};

struct SpanId {
  uint64_t value{0};

  [[nodiscard]] constexpr bool is_valid() const {
    return value != 0;
  }
  [[nodiscard]] std::string to_hex() const;
};

using Baggage = std::map<std::string, std::string>;

struct Context {
  TraceId trace_id;
  SpanId span_id;
  uint8_t trace_flags{TraceFlags::NONE};
  Baggage baggage;

  [[nodiscard]] constexpr bool is_valid() const {
    return trace_id.is_valid();
  }
  [[nodiscard]] constexpr bool is_sampled() const {
    return trace_flags & TraceFlags::SAMPLED;
  }

  void set_sampled(bool sampled) {
    if (sampled) {
      trace_flags |= TraceFlags::SAMPLED;
    } else {
      trace_flags &= ~TraceFlags::SAMPLED;
    }
  }

  [[nodiscard]] static Context create();
  [[nodiscard]] Context child(SpanId new_span) const;

  [[nodiscard]] std::string to_traceparent() const;
  [[nodiscard]] static Context from_traceparent(const std::string &header);

  [[nodiscard]] std::string to_baggage_header() const;
  static void parse_baggage(Context &ctx, const std::string &header);
};

} // namespace astra::observability

// Backward compatibility alias
namespace obs = astra::observability;
