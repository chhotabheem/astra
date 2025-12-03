#pragma once
#include "observability/IConfigLogger.h"
#include "Logger.h"

namespace config {

class DefaultConfigLogger : public IConfigLogger {
public:
    void debug(const std::string& message) override {
        logger::Logger::debug(message);
    }
    
    void info(const std::string& message) override {
        logger::Logger::info(message);
    }
    
    void warn(const std::string& message) override {
        logger::Logger::warn(message);
    }
    
    void error(const std::string& message) override {
        logger::Logger::error(message);
    }
};

}
