#pragma once

#include "Job.h"

namespace astra::concurrency {

/**
 * @brief Interface for a Worker Thread Pool.
 * 
 * Supports different topologies (Sharded, Shared) behind the same API.
 */
class IWorkerPool {
public:
    virtual ~IWorkerPool() = default;

    /**
     * @brief Starts the worker threads.
     */
    virtual void start() = 0;

    /**
     * @brief Stops the worker threads and waits for them to join.
     */
    virtual void stop() = 0;

    /**
     * @brief Submits a job to the pool.
     * 
     * @param job The job to process.
     * @return true if submitted successfully, false if queue is full or pool is stopped.
     */
    virtual bool submit(Job job) = 0;
};

} // namespace astra::concurrency
