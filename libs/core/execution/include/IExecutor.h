#pragma once

#include <functional>

namespace astra::execution {

class IExecutor {
public:
    virtual ~IExecutor() = default;
    virtual void submit(std::function<void()> task) = 0;
};

} // namespace astra::execution
