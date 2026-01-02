#pragma once

#include "Message.h"

namespace astra::execution {

/**
 * @brief Interface for handling messages delivered by StickyQueue.
 *
 * StickyQueue delivers messages to handler. Handler:
 * - Casts msg.payload to application-specific type
 * - Dispatches based on payload type (e.g., using std::visit)
 * - Processes the message
 */
class IMessageHandler {
public:
  virtual ~IMessageHandler() = default;

  /**
   * @brief Handle a message delivered by the queue.
   *
   * Called on worker thread.
   *
   * @param msg The message to handle (non-const for move semantics)
   */
  virtual void handle(Message &msg) = 0;
};

} // namespace astra::execution
