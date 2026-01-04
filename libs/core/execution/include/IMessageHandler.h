#pragma once

#include "Message.h"

namespace astra::execution {

class IMessageHandler {
public:
  virtual ~IMessageHandler() = default;

  virtual void handle(Message &msg) = 0;
};

} // namespace astra::execution
