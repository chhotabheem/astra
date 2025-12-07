#pragma once
#include "IConfigSource.h"
#include <filesystem>
#include <thread>
#include <atomic>
#include "IExecutor.h"
#include <memory>
#include <mutex>
#include <condition_variable>

namespace config {

class FileConfigSource : public IConfigSource {
public:
    explicit FileConfigSource(
        std::filesystem::path config_file_path,
        std::shared_ptr<astra::execution::IExecutor> executor = nullptr
    );
    ~FileConfigSource() override;
    
    std::string fetchConfig() override;
    void watchForChanges(ChangeCallback callback) override;
    void start() override;
    void stop() override;
    
private:
    void watchLoop();
    
    std::filesystem::path m_config_file_path;
    std::filesystem::file_time_type m_last_modified;
    ChangeCallback m_callback;
    
    std::shared_ptr<astra::execution::IExecutor> m_executor;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_task_active{false};
    std::condition_variable m_stop_cv;
    std::mutex m_stop_mutex;
    
    // inotify file descriptors
    int m_inotify_fd{-1};
    int m_watch_fd{-1};
    
    // epoll and eventfd for shutdown signaling
    int m_epoll_fd{-1};
    int m_stop_event_fd{-1};
    int m_timer_fd{-1};
};

}
