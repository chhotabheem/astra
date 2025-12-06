#pragma once
// =============================================================================
// obs/Log.h - Structured logging with trace correlation
// =============================================================================

#include "IBackend.h"  // For Level enum and Context
#include <string_view>

namespace obs {

/// Log with explicit context (for trace correlation)
void log(Level level, std::string_view message, const Context& ctx);

/// Log without context (standalone log)
void log(Level level, std::string_view message);

/// Convenience functions with context
inline void trace(std::string_view msg, const Context& ctx) { log(Level::TRACE, msg, ctx); }
inline void debug(std::string_view msg, const Context& ctx) { log(Level::DEBUG, msg, ctx); }
inline void info(std::string_view msg, const Context& ctx)  { log(Level::INFO, msg, ctx); }
inline void warn(std::string_view msg, const Context& ctx)  { log(Level::WARN, msg, ctx); }
inline void error(std::string_view msg, const Context& ctx) { log(Level::ERROR, msg, ctx); }
inline void fatal(std::string_view msg, const Context& ctx) { log(Level::FATAL, msg, ctx); }

/// Convenience functions without context
inline void trace(std::string_view msg) { log(Level::TRACE, msg); }
inline void debug(std::string_view msg) { log(Level::DEBUG, msg); }
inline void info(std::string_view msg)  { log(Level::INFO, msg); }
inline void warn(std::string_view msg)  { log(Level::WARN, msg); }
inline void error(std::string_view msg) { log(Level::ERROR, msg); }
inline void fatal(std::string_view msg) { log(Level::FATAL, msg); }

} // namespace obs
