#pragma once
#include "IConfigParser.h"

namespace config {

class JsonConfigParser : public IConfigParser {
public:
    Config parse(const std::string& raw_config) const override;
};

}
