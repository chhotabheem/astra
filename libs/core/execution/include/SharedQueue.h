#pragma once

#include "IQueue.h"
#include "execution.pb.h"
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace astra::execution {

/**
 * @brief SharedQueue - single queue with multiple worker threads.
 * 
 * Workers consume messages from a shared queue. No session affinity.
 * Use when work distribution doesn't require session stickiness.
 */
class SharedQueue : public IQueue {
public:
    explicit SharedQueue(const ::execution::SharedQueueConfig& config);
    SharedQueue(size_t num_workers, size_t max_messages = 10000);
    ~SharedQueue();

    void start();
    void stop();
    bool submit(Message msg) override;

private:
    void worker_loop();

    size_t m_num_workers;
    size_t m_max_messages;
    
    std::vector<std::thread> m_workers;
    std::queue<Message> m_queue;
    
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_running{false};
};

} // namespace astra::execution

