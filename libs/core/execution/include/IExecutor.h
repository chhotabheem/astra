#pragma once

#include "Message.h"

namespace astra::execution {

class IExecutor {
public:
  virtual ~IExecutor() = default;

  virtual void submit(Message msg) = 0;
};

} // namespace astra::execution
