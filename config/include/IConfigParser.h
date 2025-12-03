#pragma once
#include "ConfigStructs.h"
#include <string>

namespace config {

class IConfigParser {
public:
    virtual ~IConfigParser() = default;
    
    virtual Config parse(const std::string& raw_config) const = 0;
};

}
