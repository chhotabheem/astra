#pragma once
#include "observability/IConfigLogger.h"
#include <obs/Log.h>

namespace config {

class DefaultConfigLogger : public IConfigLogger {
public:
    void debug(const std::string& message) override {
        obs::debug(message);
    }
    
    void info(const std::string& message) override {
        obs::info(message);
    }
    
    void warn(const std::string& message) override {
        obs::warn(message);
    }
    
    void error(const std::string& message) override {
        obs::error(message);
    }
};

}
