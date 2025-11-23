#pragma once

#include <string>

#include <string_view>

namespace logger {

enum class Level {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

/**
 * @brief Main logger class
 * 
 * All methods are static. Thread-safe and async by default.
 */
class Logger {
public:
    static void initialize();
    static void shutdown();
    static void set_level(Level level);

    static void trace(const std::string& message) {
        log_impl(Level::TRACE, message, __FILE__, __LINE__, __FUNCTION__);
    }

    static void debug(const std::string& message) {
        log_impl(Level::DEBUG, message, __FILE__, __LINE__, __FUNCTION__);
    }

    static void info(const std::string& message) {
        log_impl(Level::INFO, message, __FILE__, __LINE__, __FUNCTION__);
    }

    static void warn(const std::string& message) {
        log_impl(Level::WARN, message, __FILE__, __LINE__, __FUNCTION__);
    }

    static void error(const std::string& message) {
        log_impl(Level::ERROR, message, __FILE__, __LINE__, __FUNCTION__);
    }

    static void fatal(const std::string& message) {
        log_impl(Level::FATAL, message, __FILE__, __LINE__, __FUNCTION__);
    }

private:
    static void log_impl(Level level, const std::string& message,
                        std::string_view file, int line, std::string_view function);
};

#define LOG_TRACE(msg) logger::Logger::trace(msg)
#define LOG_DEBUG(msg) logger::Logger::debug(msg)
#define LOG_INFO(msg) logger::Logger::info(msg)
#define LOG_WARN(msg) logger::Logger::warn(msg)
#define LOG_ERROR(msg) logger::Logger::error(msg)
#define LOG_FATAL(msg) logger::Logger::fatal(msg)

} // namespace logger
