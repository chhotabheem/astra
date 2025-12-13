#pragma once

#include "Message.h"

namespace astra::execution {

/**
 * @brief Interface for SharedQueue - single queue with multiple worker threads.
 */
class ISharedQueue {
public:
    virtual ~ISharedQueue() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool submit(Message msg) = 0;
};

} // namespace astra::execution
