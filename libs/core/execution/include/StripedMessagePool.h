#pragma once

#include "Message.h"
#include "IMessageHandler.h"
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <deque>
#include <mutex>
#include <condition_variable>

namespace astra::execution {

/**
 * @brief Striped message pool with session affinity.
 * 
 * Key difference from StripedThreadPool:
 * - Pool DELIVERS messages to handler (doesn't execute tasks)
 * - Handler decides what to do with the message
 * 
 * Session Affinity:
 * - Messages with same session_id go to same worker
 * - Ensures ordering within a session
 * - Uses modulo: worker_index = session_id % num_threads
 */
class StripedMessagePool {
public:
    /**
     * @brief Construct a striped message pool.
     * @param num_threads Number of worker threads
     * @param handler Handler that receives delivered messages
     */
    StripedMessagePool(size_t num_threads, IMessageHandler& handler);
    
    ~StripedMessagePool();
    
    // Non-copyable, non-movable
    StripedMessagePool(const StripedMessagePool&) = delete;
    StripedMessagePool& operator=(const StripedMessagePool&) = delete;
    StripedMessagePool(StripedMessagePool&&) = delete;
    StripedMessagePool& operator=(StripedMessagePool&&) = delete;
    
    /**
     * @brief Start worker threads.
     */
    void start();
    
    /**
     * @brief Stop worker threads (processes pending messages first).
     */
    void stop();
    
    /**
     * @brief Submit a message for delivery.
     * 
     * Message is routed to worker based on session_id % num_threads.
     * 
     * @param msg Message to deliver
     * @return true if submitted successfully, false if pool is stopped
     */
    bool submit(Message msg);
    
    /**
     * @brief Get number of worker threads.
     */
    [[nodiscard]] size_t thread_count() const { return m_num_threads; }

private:
    struct Worker {
        std::thread thread;
        std::deque<Message> queue;
        std::mutex mutex;
        std::condition_variable cv;
    };
    
    void worker_loop(size_t index);
    size_t select_worker(uint64_t session_id) const;
    
    size_t m_num_threads;
    std::vector<std::unique_ptr<Worker>> m_workers;
    IMessageHandler& m_handler;
    std::atomic<bool> m_running{false};
};

} // namespace astra::execution
