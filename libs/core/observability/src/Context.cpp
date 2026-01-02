// =============================================================================
// Context.cpp - Implementation of trace context
// =============================================================================
#include <Context.h>
#include <cstring>
#include <iomanip>
#include <random>
#include <sstream>

namespace astra::observability {

namespace {
// Thread-local random generator for trace/span ID generation
std::mt19937_64 &get_random_engine() {
  thread_local std::mt19937_64 engine{std::random_device{}()};
  return engine;
}

uint64_t random_uint64() {
  return get_random_engine()();
}

// Convert hex char to nibble
uint8_t hex_to_nibble(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return 10 + (c - 'a');
  }
  if (c >= 'A' && c <= 'F') {
    return 10 + (c - 'A');
  }
  return 0;
}

// Parse 16 hex chars into uint64_t
uint64_t parse_hex64(const char *str) {
  uint64_t result = 0;
  for (int i = 0; i < 16; ++i) {
    result = (result << 4) | hex_to_nibble(str[i]);
  }
  return result;
}
} // namespace

// -----------------------------------------------------------------------------
// TraceId
// -----------------------------------------------------------------------------
std::string TraceId::to_hex() const {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  oss << std::setw(16) << high << std::setw(16) << low;
  return oss.str();
}

// -----------------------------------------------------------------------------
// SpanId
// -----------------------------------------------------------------------------
std::string SpanId::to_hex() const {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0') << std::setw(16) << value;
  return oss.str();
}

// -----------------------------------------------------------------------------
// Context
// -----------------------------------------------------------------------------
Context Context::create() {
  Context ctx;
  ctx.trace_id.high = random_uint64();
  ctx.trace_id.low = random_uint64();
  ctx.span_id.value = 0;                 // Root context has no span yet
  ctx.trace_flags = TraceFlags::SAMPLED; // Sampled by default
  return ctx;
}

Context Context::child(SpanId new_span) const {
  Context ctx = *this; // Copy trace_id, trace_flags, baggage
  ctx.span_id = new_span;
  return ctx;
}

std::string Context::to_traceparent() const {
  // Format: 00-{trace_id}-{span_id}-{flags}
  std::ostringstream oss;
  oss << "00-" << trace_id.to_hex() << "-" << span_id.to_hex() << "-"
      << std::hex << std::setfill('0') << std::setw(2)
      << static_cast<int>(trace_flags);
  return oss.str();
}

Context Context::from_traceparent(const std::string &header) {
  // Format: 00-{trace_id:32}-{span_id:16}-{flags:2}
  // Length: 2 + 1 + 32 + 1 + 16 + 1 + 2 = 55
  if (header.length() < 55) {
    return Context{}; // Invalid
  }

  // Verify version (must be "00")
  if (header[0] != '0' || header[1] != '0') {
    return Context{};
  }

  // Verify delimiters
  if (header[2] != '-' || header[35] != '-' || header[52] != '-') {
    return Context{};
  }

  Context ctx;

  // Parse trace_id (32 hex chars = 128 bits)
  ctx.trace_id.high = parse_hex64(header.data() + 3);
  ctx.trace_id.low = parse_hex64(header.data() + 3 + 16);

  // Parse span_id (16 hex chars = 64 bits)
  ctx.span_id.value = parse_hex64(header.data() + 36);

  // Parse flags (2 hex chars)
  ctx.trace_flags =
      (hex_to_nibble(header[53]) << 4) | hex_to_nibble(header[54]);

  return ctx;
}

std::string Context::to_baggage_header() const {
  if (baggage.empty()) {
    return "";
  }

  std::ostringstream oss;
  bool first = true;
  for (const auto &[key, value] : baggage) {
    if (!first) {
      oss << ",";
    }
    oss << key << "=" << value;
    first = false;
  }
  return oss.str();
}

void Context::parse_baggage(Context &ctx, const std::string &header) {
  // Simple parsing: key=value,key=value
  size_t pos = 0;
  while (pos < header.length()) {
    size_t eq = header.find('=', pos);
    if (eq == std::string::npos) {
      break;
    }

    size_t comma = header.find(',', eq);
    if (comma == std::string::npos) {
      comma = header.length();
    }

    std::string key(header.substr(pos, eq - pos));
    std::string value(header.substr(eq + 1, comma - eq - 1));
    ctx.baggage[key] = value;

    pos = comma + 1;
  }
}

} // namespace astra::observability
