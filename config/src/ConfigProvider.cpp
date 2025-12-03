#include "ConfigProvider.h"
#include "ConfigValidator.h"
#include "observability/NullLogger.h"
#include "observability/NullMetrics.h"
#include <atomic>
#include <sstream>

namespace config {

astra::Result<ConfigProvider, std::string> ConfigProvider::create(
    std::unique_ptr<IConfigSource> source,
    std::unique_ptr<IConfigParser> parser,
    std::shared_ptr<IConfigLogger> logger,
    std::shared_ptr<IConfigMetrics> metrics
) {
    if (!logger) {
        logger = std::make_shared<NullLogger>();
    }
    if (!metrics) {
        metrics = std::make_shared<NullMetrics>();
    }
    
    logger->info("ConfigProvider: Loading initial configuration");
    
    try {
        std::string raw_config = source->fetchConfig();
        Config initial_config = parser->parse(raw_config);
        ConfigValidator::validate(initial_config);
        
        auto config_ptr = std::make_shared<const Config>(std::move(initial_config));
        
        logger->info("ConfigProvider: Initial configuration loaded successfully");
        
        return astra::Result<ConfigProvider, std::string>::Ok(ConfigProvider(
            std::move(source),
            std::move(parser),
            logger,
            metrics,
            config_ptr
        ));
        
    } catch (const std::exception& e) {
        std::string error_msg = "ConfigProvider: Failed to load initial configuration: ";
        error_msg += e.what();
        logger->error(error_msg);
        return astra::Result<ConfigProvider, std::string>::Err(error_msg);
    }
}

ConfigProvider::ConfigProvider(ConfigProvider&& other) noexcept
    : m_source(std::move(other.m_source))
    , m_parser(std::move(other.m_parser))
    , m_logger(std::move(other.m_logger))
    , m_metrics(std::move(other.m_metrics))
    , m_config(std::move(other.m_config))
    , m_callbacks(std::move(other.m_callbacks))
    , m_running(other.m_running)
{
    other.m_running = false;
    
    // Re-register callback with new 'this'
    if (m_source) {
        m_source->watchForChanges([this](const std::string& new_config) {
            handleConfigChange(new_config);
        });
    }
}

ConfigProvider& ConfigProvider::operator=(ConfigProvider&& other) noexcept {
    if (this != &other) {
        stop(); // Stop current before overwriting
        
        m_source = std::move(other.m_source);
        m_parser = std::move(other.m_parser);
        m_logger = std::move(other.m_logger);
        m_metrics = std::move(other.m_metrics);
        m_config = std::move(other.m_config);
        m_callbacks = std::move(other.m_callbacks);
        m_running = other.m_running;
        
        other.m_running = false;
        
        // Re-register callback with new 'this'
        if (m_source) {
            m_source->watchForChanges([this](const std::string& new_config) {
                handleConfigChange(new_config);
            });
        }
    }
    return *this;
}

ConfigProvider::ConfigProvider(
    std::unique_ptr<IConfigSource> source,
    std::unique_ptr<IConfigParser> parser,
    std::shared_ptr<IConfigLogger> logger,
    std::shared_ptr<IConfigMetrics> metrics,
    std::shared_ptr<const Config> initial_config
)
    : m_source(std::move(source))
    , m_parser(std::move(parser))
    , m_logger(std::move(logger))
    , m_metrics(std::move(metrics))
    , m_config(std::move(initial_config))
{
    m_source->watchForChanges([this](const std::string& new_config) {
        handleConfigChange(new_config);
    });
}

ConfigProvider::~ConfigProvider() {
    stop();
}

std::shared_ptr<const Config> ConfigProvider::get() const {
    return std::atomic_load(&m_config);
}

void ConfigProvider::onUpdate(UpdateCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbacks_mutex);
    m_callbacks.push_back(std::move(callback));
}

void ConfigProvider::start() {
    if (m_running) {
        m_logger->warn("ConfigProvider: Already running, ignoring start request");
        return;
    }
    
    m_running = true;
    m_source->start();
    m_logger->info("ConfigProvider: Config watching started");
}

void ConfigProvider::stop() {
    if (!m_running) {
        return;
    }
    
    m_running = false;
    m_source->stop();
    m_logger->info("ConfigProvider: Config watching stopped");
}

void ConfigProvider::handleConfigChange(const std::string& raw_config) {
    m_logger->info("ConfigProvider: Config change detected");
    
    try {
        Config new_config = m_parser->parse(raw_config);
        ConfigValidator::validate(new_config);
        
        auto new_config_ptr = std::make_shared<const Config>(std::move(new_config));
        std::atomic_store(&m_config, new_config_ptr);
        
        m_metrics->incrementReloadSuccess();
        m_logger->info("ConfigProvider: Config reloaded successfully");
        
        notifyCallbacks(*new_config_ptr);
        
    } catch (const std::exception& e) {
        m_metrics->incrementReloadFailure();
        std::string error_msg = "ConfigProvider: Config reload failed, keeping old config: ";
        error_msg += e.what();
        m_logger->error(error_msg);
    }
}

void ConfigProvider::notifyCallbacks(const Config& new_config) {
    std::vector<UpdateCallback> callbacks_copy;
    {
        std::lock_guard<std::mutex> lock(m_callbacks_mutex);
        callbacks_copy = m_callbacks;
    }
    
    m_logger->info("ConfigProvider: Notifying " + std::to_string(callbacks_copy.size()) + " callback(s)");
    
    for (const auto& callback : callbacks_copy) {
        try {
            callback(new_config);
        } catch (const std::exception& e) {
            std::string error_msg = "ConfigProvider: Callback failed: ";
            error_msg += e.what();
            m_logger->error(error_msg);
        }
    }
}

}
