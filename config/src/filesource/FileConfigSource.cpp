#include "filesource/FileConfigSource.h"
#include <obs/Log.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <sys/inotify.h>
#include <unistd.h>
#include <cstring>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <cerrno>
#include <mutex>
#include <condition_variable>
#include "filesource/InotifyIterator.h"
#include "concurrency/include/Executors.h"

namespace config {

FileConfigSource::FileConfigSource(
    std::filesystem::path config_file_path,
    std::shared_ptr<astra::concurrency::IExecutor> executor
)
    : m_config_file_path(std::move(config_file_path))
    , m_executor(std::move(executor))
    , m_inotify_fd(-1)
    , m_watch_fd(-1)
    , m_timer_fd(-1)
{
    if (!m_executor) {
        m_executor = std::make_shared<astra::concurrency::ThreadExecutor>();
    }
    
    obs::info("FileConfigSource: Initialized with file: " + m_config_file_path.string());
    
    if (std::filesystem::exists(m_config_file_path)) {
        m_last_modified = std::filesystem::last_write_time(m_config_file_path);
    } else {
        obs::warn("FileConfigSource: Config file does not exist yet: " + m_config_file_path.string());
    }
}

FileConfigSource::~FileConfigSource() {
    stop();
}

std::string FileConfigSource::fetchConfig() {
    if (!std::filesystem::exists(m_config_file_path)) {
        throw std::runtime_error("Config file does not exist: " + m_config_file_path.string());
    }
    
    std::ifstream file(m_config_file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + m_config_file_path.string());
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    obs::debug("FileConfigSource: Read " + std::to_string(buffer.str().size()) + " bytes from " + m_config_file_path.string());
    
    return buffer.str();
}

void FileConfigSource::watchForChanges(ChangeCallback callback) {
    m_callback = std::move(callback);
}

#include <sys/epoll.h>
#include <sys/eventfd.h>

void FileConfigSource::start() {
    if (m_running.load()) {
        obs::warn("FileConfigSource: Already running");
        return;
    }
    
    // Initialize inotify
    m_inotify_fd = inotify_init1(IN_NONBLOCK); // Non-blocking for epoll
    if (m_inotify_fd == -1) {
        obs::error("FileConfigSource: Failed to initialize inotify: " + std::string(std::strerror(errno)));
        return;
    }
    
    // Watch the parent directory
    auto parent_dir = m_config_file_path.parent_path();
    m_watch_fd = inotify_add_watch(m_inotify_fd, parent_dir.c_str(), 
                                    IN_MODIFY | IN_MOVED_TO | IN_CREATE | IN_DELETE_SELF);
    
    if (m_watch_fd == -1) {
        obs::error("FileConfigSource: Failed to add inotify watch: " + std::string(std::strerror(errno)));
        close(m_inotify_fd);
        m_inotify_fd = -1;
        return;
    }
    
    // Create eventfd for stop signal
    m_stop_event_fd = eventfd(0, EFD_NONBLOCK);
    if (m_stop_event_fd == -1) {
        obs::error("FileConfigSource: Failed to create eventfd: " + std::string(std::strerror(errno)));
        close(m_inotify_fd);
        m_inotify_fd = -1;
        return;
    }
    
    // Create epoll instance
    m_epoll_fd = epoll_create1(0);
    if (m_epoll_fd == -1) {
        obs::error("FileConfigSource: Failed to create epoll: " + std::string(std::strerror(errno)));
        close(m_stop_event_fd);
        close(m_inotify_fd);
        m_stop_event_fd = -1;
        m_inotify_fd = -1;
        return;
    }
    
    // Create timerfd for debounce
    m_timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (m_timer_fd == -1) {
        obs::error("FileConfigSource: Failed to create timerfd: " + std::string(std::strerror(errno)));
        close(m_stop_event_fd);
        close(m_epoll_fd);
        close(m_inotify_fd);
        return;
    }

    // Add inotify fd to epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = m_inotify_fd;
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_inotify_fd, &ev) == -1) {
        obs::error("FileConfigSource: Failed to add inotify fd to epoll: " + std::string(std::strerror(errno)));
        close(m_timer_fd);
        close(m_stop_event_fd);
        close(m_epoll_fd);
        close(m_inotify_fd);
        return;
    }
    
    // Add stop event fd to epoll
    ev.events = EPOLLIN;
    ev.data.fd = m_stop_event_fd;
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_stop_event_fd, &ev) == -1) {
        obs::error("FileConfigSource: Failed to add event fd to epoll: " + std::string(std::strerror(errno)));
        close(m_timer_fd);
        close(m_stop_event_fd);
        close(m_epoll_fd);
        close(m_inotify_fd);
        return;
    }

    // Add timer fd to epoll
    ev.events = EPOLLIN;
    ev.data.fd = m_timer_fd;
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_timer_fd, &ev) == -1) {
        obs::error("FileConfigSource: Failed to add timer fd to epoll: " + std::string(std::strerror(errno)));
        close(m_timer_fd);
        close(m_stop_event_fd);
        close(m_epoll_fd);
        close(m_inotify_fd);
        return;
    }
    
    m_running.store(true);
    m_task_active.store(true);
    
    // Use executor to submit the watch task
    m_executor->submit([this]() {
        this->watchLoop();
        // Hold lock during the entire notification sequence
        // This ensures notify_all completes before wait() returns in stop()
        std::lock_guard<std::mutex> lock(m_stop_mutex);
        m_task_active.store(false);
        m_stop_cv.notify_all();
    });
    
    obs::info("FileConfigSource: Started watching file with epoll: " + m_config_file_path.string());
}

void FileConfigSource::stop() {
    if (!m_running.load()) {
        return;
    }
    
    m_running.store(false);
    
    // Signal stop via eventfd
    if (m_stop_event_fd != -1) {
        uint64_t u = 1;
        write(m_stop_event_fd, &u, sizeof(uint64_t));
    }
    
    // Wait for task to finish using condition variable (no sleep!)
    std::unique_lock<std::mutex> lock(m_stop_mutex);
    m_stop_cv.wait(lock, [this] { return !m_task_active.load(); });
    
    // Cleanup FDs
    if (m_timer_fd != -1) { close(m_timer_fd); m_timer_fd = -1; }
    if (m_epoll_fd != -1) { close(m_epoll_fd); m_epoll_fd = -1; }
    if (m_stop_event_fd != -1) { close(m_stop_event_fd); m_stop_event_fd = -1; }
    if (m_inotify_fd != -1) { close(m_inotify_fd); m_inotify_fd = -1; }
    
    obs::info("FileConfigSource: Stopped watching file");
}

void FileConfigSource::watchLoop() {
    constexpr size_t EVENT_SIZE = sizeof(struct inotify_event);
    constexpr size_t BUF_LEN = 1024 * (EVENT_SIZE + 16);
    char buffer[BUF_LEN];
    struct epoll_event events[2];
    
    auto filename = m_config_file_path.filename().string();
    
    while (m_running.load()) {
        // Wait indefinitely for events
        int nfds = epoll_wait(m_epoll_fd, events, 2, -1);
        
        if (nfds == -1) {
            if (errno == EINTR) continue;
            obs::error("FileConfigSource: epoll_wait error: " + std::string(std::strerror(errno)));
            break;
        }
        
        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == m_stop_event_fd) {
                // Stop signal received
                return;
            }
            
            if (events[n].data.fd == m_timer_fd) {
                // Timer expired (debounce complete)
                uint64_t expirations;
                read(m_timer_fd, &expirations, sizeof(expirations));
                
                if (m_callback) {
                    obs::info("FileConfigSource: File modification detected (debounced): " + m_config_file_path.string());
                    try {
                        std::string new_content = fetchConfig();
                        m_callback(new_content);
                        if (std::filesystem::exists(m_config_file_path)) {
                            m_last_modified = std::filesystem::last_write_time(m_config_file_path);
                        }
                    } catch (const std::exception& e) {
                        obs::error("FileConfigSource: Failed to read changed file: " + std::string(e.what()));
                    }
                }
            } else if (events[n].data.fd == m_inotify_fd) {
                // Inotify event
                ssize_t length = read(m_inotify_fd, buffer, BUF_LEN);
                if (length < 0) {
                    if (errno != EAGAIN) {
                        obs::error("FileConfigSource: inotify read error: " + std::string(std::strerror(errno)));
                    }
                    continue;
                }
                
                // Process events
                bool file_changed = false;
                
                for (const auto& event : InotifyEvents(buffer, length)) {
                    if (event.len > 0 && event.name == filename) {
                        if (event.mask & (IN_MODIFY | IN_MOVED_TO | IN_CREATE)) {
                            file_changed = true;
                        }
                        if (event.mask & IN_DELETE_SELF) {
                            obs::warn("FileConfigSource: Config file deleted: " + m_config_file_path.string());
                        }
                    }
                }
                
                if (file_changed) {
                    // Arm/Rearm timer for 10ms debounce
                    struct itimerspec ts;
                    ts.it_interval.tv_sec = 0;
                    ts.it_interval.tv_nsec = 0; // One-shot
                    ts.it_value.tv_sec = 0;
                    ts.it_value.tv_nsec = 10 * 1000 * 1000; // 10ms
                    
                    if (timerfd_settime(m_timer_fd, 0, &ts, nullptr) == -1) {
                        obs::error("FileConfigSource: Failed to arm timer: " + std::string(std::strerror(errno)));
                    }
                }
            }
        }
    }
}

}
