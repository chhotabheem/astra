#pragma once

#include "ShortCode.h"

namespace uri_shortener::domain {

class ICodeGenerator {
public:
  virtual ~ICodeGenerator() = default;
  virtual ShortCode generate() = 0;
};

} // namespace uri_shortener::domain
