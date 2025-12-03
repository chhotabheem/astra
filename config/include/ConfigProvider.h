#pragma once
#include "ConfigStructs.h"
#include "IConfigSource.h"
#include "IConfigParser.h"
#include "observability/IConfigLogger.h"
#include "observability/IConfigMetrics.h"
#include <memory>
#include <functional>
#include <vector>
#include <mutex>
#include "outcome/include/Result.h"

namespace config {

class ConfigProvider {
public:
    using UpdateCallback = std::function<void(const Config&)>;
    
    static astra::Result<ConfigProvider, std::string> create(
        std::unique_ptr<IConfigSource> source,
        std::unique_ptr<IConfigParser> parser,
        std::shared_ptr<IConfigLogger> logger = nullptr,
        std::shared_ptr<IConfigMetrics> metrics = nullptr
    );
    
    ~ConfigProvider();
    
    ConfigProvider(ConfigProvider&& other) noexcept;
    ConfigProvider& operator=(ConfigProvider&& other) noexcept;
    
    // Delete copy
    ConfigProvider(const ConfigProvider&) = delete;
    ConfigProvider& operator=(const ConfigProvider&) = delete;
    
    std::shared_ptr<const Config> get() const;
    void onUpdate(UpdateCallback callback);
    void start();
    void stop();
    
    ConfigProvider(
        std::unique_ptr<IConfigSource> source,
        std::unique_ptr<IConfigParser> parser,
        std::shared_ptr<IConfigLogger> logger,
        std::shared_ptr<IConfigMetrics> metrics,
        std::shared_ptr<const Config> initial_config
    );
    
private:
    void handleConfigChange(const std::string& raw_config);
    void notifyCallbacks(const Config& new_config);
    
    std::unique_ptr<IConfigSource> m_source;
    std::unique_ptr<IConfigParser> m_parser;
    std::shared_ptr<IConfigLogger> m_logger;
    std::shared_ptr<IConfigMetrics> m_metrics;
    std::shared_ptr<const Config> m_config;
    std::vector<UpdateCallback> m_callbacks;
    mutable std::mutex m_callbacks_mutex;
    bool m_running{false};
};

}
