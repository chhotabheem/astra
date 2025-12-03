#pragma once
#include "observability/IConfigLogger.h"

namespace config {

class NullLogger : public IConfigLogger {
public:
    void debug(const std::string&) override {}
    void info(const std::string&) override {}
    void warn(const std::string&) override {}
    void error(const std::string&) override {}
};

}
