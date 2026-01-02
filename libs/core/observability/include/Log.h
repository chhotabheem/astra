#pragma once

#include <initializer_list>
#include <map>
#include <string>
#include <utility>

namespace astra::observability {

// Log levels (OpenTelemetry standard)
enum class Level {
  Trace = 1,  // TRACE
  Debug = 5,  // DEBUG
  Info = 9,   // INFO
  Warn = 13,  // WARN
  Error = 17, // ERROR
  Fatal = 21  // FATAL
};

// Attributes for logs
using Attributes = std::initializer_list<std::pair<std::string, std::string>>;

// Core logging function
void log(Level level, const std::string &message, Attributes attrs = {});

// Convenience functions
inline void trace(const std::string &msg, Attributes attrs = {}) {
  log(Level::Trace, msg, attrs);
}

inline void debug(const std::string &msg, Attributes attrs = {}) {
  log(Level::Debug, msg, attrs);
}

inline void info(const std::string &msg, Attributes attrs = {}) {
  log(Level::Info, msg, attrs);
}

inline void warn(const std::string &msg, Attributes attrs = {}) {
  log(Level::Warn, msg, attrs);
}

inline void error(const std::string &msg, Attributes attrs = {}) {
  log(Level::Error, msg, attrs);
}

inline void fatal(const std::string &msg, Attributes attrs = {}) {
  log(Level::Fatal, msg, attrs);
}

/**
 * Scoped Log Attributes (MDC Pattern)
 *
 * Usage:
 *   void handle_request(const Request& req) {
 *       ScopedLogAttributes scoped({
 *           {"request.id", req.id()},
 *           {"session.id", req.session_id()}
 *       });
 *
 *       // ALL logs in this scope inherit these attributes
 *       obs::info("Processing request");
 *       // Includes: request.id, session.id, trace_id, span_id
 *   } // Scoped attributes removed on destruction
 */
class ScopedLogAttributes {
public:
  explicit ScopedLogAttributes(Attributes attrs);
  ~ScopedLogAttributes();

  // Non-copyable, non-movable
  ScopedLogAttributes(const ScopedLogAttributes &) = delete;
  ScopedLogAttributes &operator=(const ScopedLogAttributes &) = delete;
  ScopedLogAttributes(ScopedLogAttributes &&) = delete;
  ScopedLogAttributes &operator=(ScopedLogAttributes &&) = delete;

private:
  size_t m_stack_size; // For restoring stack on destruction
};

} // namespace astra::observability

// Backward compatibility alias
namespace obs = astra::observability;
