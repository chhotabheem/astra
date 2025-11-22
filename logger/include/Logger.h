/**
 * @file log.h
 * @brief Astra Logging Library - Public API
 * 
 * Provides a simple, high-performance logging interface with JSON output.
 * Thread-safe and async by default.
 * 
 * Usage:
 *   #include <astra/log.h>
 *   
 *   log::Logger::info("Application started");
 *   log::Logger::error("Connection failed");
 */

#pragma once

#include <string>

namespace logger {

/**
 * @brief Log severity levels
 */
enum class Level {
    TRACE,   ///< Detailed trace information
    DEBUG,   ///< Debug information
    INFO,    ///< Informational messages
    WARN,    ///< Warning messages
    ERROR,   ///< Error messages
    FATAL    ///< Fatal error messages
};

/**
 * @brief Main logger class
 * 
 * All methods are static. Thread-safe and async by default.
 */
class Logger {
public:
    /**
     * @brief Initialize the logging system
     * 
     * Must be called before any logging operations.
     * Safe to call multiple times (subsequent calls are ignored).
     */
    static void initialize();

    /**
     * @brief Shutdown the logging system
     * 
     * Flushes all pending log messages and cleans up resources.
     * Should be called before program exit.
     */
    static void shutdown();

    /**
     * @brief Set the minimum log level
     * 
     * Messages below this level will be discarded.
     * 
     * @param level Minimum log level
     */
    static void set_level(Level level);

    /**
     * @brief Log a trace message
     * 
     * Automatically captures file, line, and function information.
     * 
     * @param message Log message
     */
    static void trace(const std::string& message) {
        log_impl(Level::TRACE, message, __FILE__, __LINE__, __FUNCTION__);
    }

    /**
     * @brief Log a debug message
     * 
     * Automatically captures file, line, and function information.
     * 
     * @param message Log message
     */
    static void debug(const std::string& message) {
        log_impl(Level::DEBUG, message, __FILE__, __LINE__, __FUNCTION__);
    }

    /**
     * @brief Log an info message
     * 
     * Automatically captures file, line, and function information.
     * 
     * @param message Log message
     */
    static void info(const std::string& message) {
        log_impl(Level::INFO, message, __FILE__, __LINE__, __FUNCTION__);
    }

    /**
     * @brief Log a warning message
     * 
     * Automatically captures file, line, and function information.
     * 
     * @param message Log message
     */
    static void warn(const std::string& message) {
        log_impl(Level::WARN, message, __FILE__, __LINE__, __FUNCTION__);
    }

    /**
     * @brief Log an error message
     * 
     * Automatically captures file, line, and function information.
     * 
     * @param message Log message
     */
    static void error(const std::string& message) {
        log_impl(Level::ERROR, message, __FILE__, __LINE__, __FUNCTION__);
    }

    /**
     * @brief Log a fatal error message
     * 
     * Automatically captures file, line, and function information.
     * 
     * @param message Log message
     */
    static void fatal(const std::string& message) {
        log_impl(Level::FATAL, message, __FILE__, __LINE__, __FUNCTION__);
    }

private:
    // Internal implementation (do not call directly)
    static void log_impl(Level level, const std::string& message,
                        const char* file, int line, const char* function);
};

// ============================================================================
// Macro-Based API - Recommended for application code
// ============================================================================

/**
 * @brief Log a trace message
 * 
 * Automatically captures file, line, and function information.
 * 
 * @param msg Log message (string)
 * 
 * Example: LOG_TRACE("Entering function");
 */
#define LOG_TRACE(msg) logger::Logger::trace(msg)

/**
 * @brief Log a debug message
 * 
 * Automatically captures file, line, and function information.
 * 
 * @param msg Log message (string)
 * 
 * Example: LOG_DEBUG("Variable value: " + std::to_string(x));
 */
#define LOG_DEBUG(msg) logger::Logger::debug(msg)

/**
 * @brief Log an info message
 * 
 * Automatically captures file, line, and function information.
 * 
 * @param msg Log message (string)
 * 
 * Example: LOG_INFO("Application started");
 */
#define LOG_INFO(msg) logger::Logger::info(msg)

/**
 * @brief Log a warning message
 * 
 * Automatically captures file, line, and function information.
 * 
 * @param msg Log message (string)
 * 
 * Example: LOG_WARN("Memory usage high");
 */
#define LOG_WARN(msg) logger::Logger::warn(msg)

/**
 * @brief Log an error message
 * 
 * Automatically captures file, line, and function information.
 * 
 * @param msg Log message (string)
 * 
 * Example: LOG_ERROR("Connection failed");
 */
#define LOG_ERROR(msg) logger::Logger::error(msg)

/**
 * @brief Log a fatal error message
 * 
 * Automatically captures file, line, and function information.
 * 
 * @param msg Log message (string)
 * 
 * Example: LOG_FATAL("Critical system failure");
 */
#define LOG_FATAL(msg) logger::Logger::fatal(msg)

} // namespace logger
