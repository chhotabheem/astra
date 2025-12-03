#pragma once

#include <functional>

namespace astra::concurrency {

/**
 * @brief Interface for task execution.
 * 
 * Decouples the "what" (task) from the "how" (threading model).
 * Allows injecting different executors for production (ThreadPool) vs testing (Inline).
 */
class IExecutor {
public:
    virtual ~IExecutor() = default;

    /**
     * @brief Submits a task for execution.
     * 
     * @param task The task to execute.
     */
    virtual void submit(std::function<void()> task) = 0;
};

} // namespace astra::concurrency
