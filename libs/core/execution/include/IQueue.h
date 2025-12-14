#pragma once

#include "Message.h"

namespace astra::execution {

/**
 * @brief Interface for message queues
 * 
 * Allows mocking in unit tests and provides abstraction
 * over different queue implementations (StickyQueue, SharedQueue).
 */
class IQueue {
public:
    virtual ~IQueue() = default;
    
    /**
     * @brief Submit a message for delivery.
     * @param msg Message to submit
     * @return true if submitted successfully
     */
    virtual bool submit(Message msg) = 0;
};

} // namespace astra::execution
