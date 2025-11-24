#pragma once

#include <Logger.h>
#include <memory>
#include <mutex>

#include <string_view>

// Forward declare spdlog types to avoid exposing spdlog in public headers
namespace spdlog {
    class logger;
    namespace sinks {
        class sink;
    }
}

namespace logger {
namespace internal {

class LoggerImpl {
public:
    static LoggerImpl& instance();

    void initialize();
    void shutdown();
    void set_level(Level level);
    void log(Level level, const std::string& message,
             std::string_view file, int line, std::string_view function);

    // Delete copy/move constructors
    LoggerImpl(const LoggerImpl&) = delete;
    LoggerImpl& operator=(const LoggerImpl&) = delete;
    LoggerImpl(LoggerImpl&&) = delete;
    LoggerImpl& operator=(LoggerImpl&&) = delete;

private:
    LoggerImpl();
    ~LoggerImpl();

    std::shared_ptr<spdlog::logger> m_logger;
    std::once_flag m_init_flag;
    bool m_is_initialized = false;
    bool m_is_shutdown = false;
};

} // namespace internal
} // namespace logger
