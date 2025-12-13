#pragma once

#include "ShortCode.h"

namespace url_shortener::domain {

class ICodeGenerator {
public:
    virtual ~ICodeGenerator() = default;
    virtual ShortCode generate() = 0;
};

} // namespace url_shortener::domain
