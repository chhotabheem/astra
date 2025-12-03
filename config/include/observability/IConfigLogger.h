#pragma once
#include <string>

namespace config {

class IConfigLogger {
public:
    virtual ~IConfigLogger() = default;
    
    virtual void debug(const std::string& message) =  0;
    virtual void info(const std::string& message) = 0;
    virtual void warn(const std::string& message) = 0;
    virtual void error(const std::string& message) = 0;
};

}
