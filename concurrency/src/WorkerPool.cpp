#include "WorkerPool.hpp"
#include <iostream>

namespace astra::concurrency {

WorkerPool::WorkerPool(size_t num_threads) : num_threads_(num_threads) {
    workers_.reserve(num_threads_);
    for (size_t i = 0; i < num_threads_; ++i) {
        workers_.push_back(std::make_unique<Worker>());
    }
}

WorkerPool::~WorkerPool() {
    stop();
}

void WorkerPool::start() {
    if (running_) return;
    running_ = true;

    for (size_t i = 0; i < num_threads_; ++i) {
        workers_[i]->thread = std::thread(&WorkerPool::worker_loop, this, i);
    }
}

void WorkerPool::stop() {
    if (!running_) return;
    running_ = false;

    // Wake up all workers so they can exit
    for (auto& worker : workers_) {
        {
            std::lock_guard<std::mutex> lock(worker->mutex);
        } // Flush lock
        worker->cv.notify_all();
    }

    for (auto& worker : workers_) {
        if (worker->thread.joinable()) {
            worker->thread.join();
        }
    }
}

bool WorkerPool::submit(Job job) {
    if (!running_) return false;

    // Sharding Logic: Route to specific worker based on session_id
    size_t worker_index = job.session_id % num_threads_;
    auto& worker = workers_[worker_index];

    {
        std::lock_guard<std::mutex> lock(worker->mutex);
        worker->queue.push_back(job);
    }
    worker->cv.notify_one();
    return true;
}

void WorkerPool::worker_loop(size_t index) {
    auto& worker = workers_[index];

    while (running_) {
        Job job{JobType::SHUTDOWN, 0, {}};
        {
            std::unique_lock<std::mutex> lock(worker->mutex);
            worker->cv.wait(lock, [&] { 
                return !worker->queue.empty() || !running_; 
            });

            if (!running_ && worker->queue.empty()) {
                return;
            }

            if (!worker->queue.empty()) {
                job = worker->queue.front();
                worker->queue.pop_front();
            }
        }

        // Process the job
        // In a real implementation, we would dispatch based on job.type
        // For now, we just acknowledge it.
        // TODO: Implement Job Dispatcher (Visitor or Switch)
        
        if (job.type == JobType::SHUTDOWN) continue;

        // Example processing placeholder
        // std::cout << "Worker " << index << " processing job type " << (int)job.type << " for session " << job.session_id << std::endl;
    }
}

} // namespace astra::concurrency
