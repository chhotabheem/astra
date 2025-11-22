/**
 * @file LoggerImpl.h
 * @brief Internal logger implementation using spdlog
 */

#pragma once

#include <Logger.h>
#include <memory>
#include <mutex>

// Forward declare spdlog types to avoid exposing spdlog in public headers
namespace spdlog {
    class logger;
    namespace sinks {
        class sink;
    }
}

namespace logger {
namespace internal {

/**
 * @brief Singleton logger implementation
 * 
 * Thread-safe singleton that manages the spdlog instance.
 */
class LoggerImpl {
public:
    /**
     * @brief Get the singleton instance
     */
    static LoggerImpl& instance();

    /**
     * @brief Initialize the logger
     */
    void initialize();

    /**
     * @brief Shutdown the logger
     */
    void shutdown();

    /**
     * @brief Set minimum log level
     */
    void set_level(Level level);

    /**
     * @brief Log a message with metadata
     */
    void log(Level level, const std::string& message,
             const char* file, int line, const char* function);

    // Delete copy/move constructors
    LoggerImpl(const LoggerImpl&) = delete;
    LoggerImpl& operator=(const LoggerImpl&) = delete;
    LoggerImpl(LoggerImpl&&) = delete;
    LoggerImpl& operator=(LoggerImpl&&) = delete;

private:
    LoggerImpl();
    ~LoggerImpl();

    std::shared_ptr<spdlog::logger> logger_;
    std::once_flag init_flag_;
    bool initialized_ = false;
};

} // namespace internal
} // namespace logger
