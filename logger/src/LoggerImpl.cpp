#include "LoggerImpl.h"
#include <JsonFormatter.h>
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

class JsonFormatterSpdlog : public spdlog::custom_flag_formatter {
public:
    void format(const spdlog::details::log_msg& msg, 
                const std::tm&, 
                spdlog::memory_buf_t& dest) override {
    }

    std::unique_ptr<custom_flag_formatter> clone() const override {
        return std::make_unique<JsonFormatterSpdlog>();
    }
};

// Forward declare format_json helper
static std::string format_json(spdlog::level::level_enum level,
                               const std::string& message,
                               std::string_view file,
                               int line,
                               std::string_view function);

std::string format_json(spdlog::level::level_enum level,
                        const std::string& message,
                        std::string_view file,
                        int line,
                        std::string_view function) {
    json::JsonFormatter json;
    
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()) % 1000000;
    
    std::tm tm_now;
    localtime_r(&time_t_now, &tm_now);
    
    std::ostringstream timestamp;
    timestamp << std::put_time(&tm_now, "%Y-%m-%dT%H:%M:%S")
              << "." << std::setfill('0') << std::setw(6) << microseconds.count();

    std::ostringstream thread_id_stream;
    thread_id_stream << std::this_thread::get_id();
    
    const char* level_str = spdlog::level::to_string_view(level).data();
    
    std::string_view filename = file;
    size_t last_slash = file.find_last_of('/');
    if (last_slash != std::string_view::npos) {
        filename = file.substr(last_slash + 1);
    }

    json.add("timestamp", timestamp.str());
    json.add("level", level_str);
    json.add("message", message);
    
    json.start_object("source");
        json.add("file", filename);
        json.add("line", line);
        json.add("function", function);
    json.end_object();
    
    json.add("thread_id", thread_id_stream.str());
    json.end_object();
    
    return json.get_string();
}

LoggerImpl& LoggerImpl::instance() {
    static LoggerImpl instance;
    return instance;
}

LoggerImpl::LoggerImpl() {
}

LoggerImpl::~LoggerImpl() {
    shutdown();
}

void LoggerImpl::initialize() {
    std::call_once(m_init_flag, [this]() {
        try {
            // Create async logger with queue size 8192
            spdlog::init_thread_pool(8192, 1);
            
            // Create stdout sink
            auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            
            // Create async logger
            m_logger = std::make_shared<spdlog::async_logger>(
                "astra_logger",
                stdout_sink,
                spdlog::thread_pool(),
                spdlog::async_overflow_policy::block
            );
            
            // Set pattern to just output the message (we format JSON ourselves)
            m_logger->set_pattern("%v");
            
            // Set default level to INFO
            m_logger->set_level(spdlog::level::info);
            
            // Register logger
            spdlog::register_logger(m_logger);
            
            m_is_initialized = true;
        } catch (const std::exception& e) {
            // Fallback to stderr if initialization fails
            std::cerr << "Failed to initialize logger: " << e.what() << std::endl;
        }
    });
}

void LoggerImpl::shutdown() {
    if (m_is_initialized && m_logger) {
        m_logger->flush();
        spdlog::drop("astra_logger");
        spdlog::shutdown();
        m_is_initialized = false;
    }
}

void LoggerImpl::set_level(Level level) {
    if (!m_is_initialized) {
        initialize();
    }
    
    if (m_logger) {
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
        m_logger->set_level(spdlog_level);
    }
}

void LoggerImpl::log(Level level, const std::string& message,
                     std::string_view file, int line, std::string_view function) {
    if (!m_is_initialized) {
        initialize();
    }
    
    if (!m_logger) {
        return;
    }
    
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
    
    std::string json_message = format_json(spdlog_level, message, file, line, function);
    m_logger->log(spdlog_level, json_message);
}

} // namespace internal

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
                     std::string_view file, int line, std::string_view function) {
    internal::LoggerImpl::instance().log(level, message, file, line, function);
}

} // namespace logger
