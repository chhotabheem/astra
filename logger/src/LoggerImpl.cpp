/**
 * @file LoggerImpl.cpp
 * @brief Logger implementation with JSON formatting
 */

#include "LoggerImpl.h"
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/pattern_formatter.h>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

namespace logger {
namespace internal {

/**
 * @brief Custom JSON formatter for spdlog
 */
class JsonFormatter : public spdlog::custom_flag_formatter {
public:
    void format(const spdlog::details::log_msg& msg, 
                const std::tm&, 
                spdlog::memory_buf_t& dest) override {
        // This is called for custom flags, but we'll override the entire format
        // in the pattern
    }

    std::unique_ptr<custom_flag_formatter> clone() const override {
        return std::make_unique<JsonFormatter>();
    }
};

/**
 * @brief Format log message as JSON
 */
std::string format_json(spdlog::level::level_enum level,
                        const std::string& message,
                        const char* file,
                        int line,
                        const char* function) {
    std::ostringstream json;
    
    // Get current timestamp with microseconds
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()) % 1000000;
    
    std::tm tm_now;
    localtime_r(&time_t_now, &tm_now);
    
    // Get thread ID
    std::ostringstream thread_id_stream;
    thread_id_stream << std::this_thread::get_id();
    std::string thread_id = thread_id_stream.str();
    
    // Level to string
    const char* level_str = spdlog::level::to_string_view(level).data();
    
    // Extract just the filename from full path
    const char* filename = file;
    const char* last_slash = strrchr(file, '/');
    if (last_slash) {
        filename = last_slash + 1;
    }
    
    // Build JSON
    json << "{"
         << "\"timestamp\":\"" << std::put_time(&tm_now, "%Y-%m-%dT%H:%M:%S")
         << "." << std::setfill('0') << std::setw(6) << microseconds.count() << "\","
         << "\"level\":\"" << level_str << "\","
         << "\"message\":\"" << message << "\","
         << "\"source\":{"
         << "\"file\":\"" << filename << "\","
         << "\"line\":" << line << ","
         << "\"function\":\"" << function << "\""
         << "},"
         << "\"thread_id\":\"" << thread_id << "\""
         << "}";
    
    return json.str();
}

// Singleton instance
LoggerImpl& LoggerImpl::instance() {
    static LoggerImpl instance;
    return instance;
}

LoggerImpl::LoggerImpl() {
    // Constructor is private
}

LoggerImpl::~LoggerImpl() {
    shutdown();
}

void LoggerImpl::initialize() {
    std::call_once(init_flag_, [this]() {
        try {
            // Create async logger with queue size 8192
            spdlog::init_thread_pool(8192, 1);
            
            // Create stdout sink
            auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            
            // Create async logger
            logger_ = std::make_shared<spdlog::async_logger>(
                "astra_logger",
                stdout_sink,
                spdlog::thread_pool(),
                spdlog::async_overflow_policy::block
            );
            
            // Set pattern to just output the message (we format JSON ourselves)
            logger_->set_pattern("%v");
            
            // Set default level to INFO
            logger_->set_level(spdlog::level::info);
            
            // Register logger
            spdlog::register_logger(logger_);
            
            initialized_ = true;
        } catch (const std::exception& e) {
            // Fallback to stderr if initialization fails
            std::cerr << "Failed to initialize logger: " << e.what() << std::endl;
        }
    });
}

void LoggerImpl::shutdown() {
    if (initialized_ && logger_) {
        logger_->flush();
        spdlog::drop("astra_logger");
        spdlog::shutdown();
        initialized_ = false;
    }
}

void LoggerImpl::set_level(Level level) {
    if (!initialized_) {
        initialize();
    }
    
    if (logger_) {
        spdlog::level::level_enum spdlog_level;
        switch (level) {
            case Level::TRACE: spdlog_level = spdlog::level::trace; break;
            case Level::DEBUG: spdlog_level = spdlog::level::debug; break;
            case Level::INFO:  spdlog_level = spdlog::level::info;  break;
            case Level::WARN:  spdlog_level = spdlog::level::warn;  break;
            case Level::ERROR: spdlog_level = spdlog::level::err;   break;
            case Level::FATAL: spdlog_level = spdlog::level::critical; break;
            default:           spdlog_level = spdlog::level::info;  break;
        }
        logger_->set_level(spdlog_level);
    }
}

void LoggerImpl::log(Level level, const std::string& message,
                     const char* file, int line, const char* function) {
    if (!initialized_) {
        initialize();
    }
    
    if (!logger_) {
        return;
    }
    
    // Convert level
    spdlog::level::level_enum spdlog_level;
    switch (level) {
        case Level::TRACE: spdlog_level = spdlog::level::trace; break;
        case Level::DEBUG: spdlog_level = spdlog::level::debug; break;
        case Level::INFO:  spdlog_level = spdlog::level::info;  break;
        case Level::WARN:  spdlog_level = spdlog::level::warn;  break;
        case Level::ERROR: spdlog_level = spdlog::level::err;   break;
        case Level::FATAL: spdlog_level = spdlog::level::critical; break;
        default:           spdlog_level = spdlog::level::info;  break;
    }
    
    // Format as JSON
    std::string json_message = format_json(spdlog_level, message, file, line, function);
    
    // Log the JSON message
    logger_->log(spdlog_level, json_message);
}

} // namespace internal

// Public API implementation
void Logger::initialize() {
    internal::LoggerImpl::instance().initialize();
}

void Logger::shutdown() {
    internal::LoggerImpl::instance().shutdown();
}

void Logger::set_level(Level level) {
    internal::LoggerImpl::instance().set_level(level);
}

void Logger::log_impl(Level level, const std::string& message,
                     const char* file, int line, const char* function) {
    internal::LoggerImpl::instance().log(level, message, file, line, function);
}

} // namespace logger
