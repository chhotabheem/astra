#pragma once

#include "Message.h"

namespace astra::execution {

/**
 * @brief Interface for handling messages delivered by StripedMessagePool.
 * 
 * Unlike task execution (pool runs the task), message handling is:
 * - Pool delivers message to handler
 * - Handler decides what to do with the message
 * - Handler can dispatch based on payload type using std::visit
 */
class IMessageHandler {
public:
    virtual ~IMessageHandler() = default;
    
    /**
     * @brief Handle a message delivered by the pool.
     * 
     * Called on worker thread. Handler should:
     * 1. Cast msg.payload to application-specific variant type
     * 2. Use std::visit to dispatch based on variant type
     * 3. Process the message accordingly
     * 
     * @param msg The message to handle (non-const for move semantics)
     */
    virtual void handle(Message& msg) = 0;
};

} // namespace astra::execution
