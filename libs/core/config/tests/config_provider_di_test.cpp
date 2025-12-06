#include <gtest/gtest.h>
#include "ConfigProvider.h"
#include "IConfigSource.h"
#include "parsers/JsonConfigParser.h"
#include "observability/IConfigLogger.h"
#include "observability/IConfigMetrics.h"
#include <memory>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>

namespace config {

class MockLogger : public IConfigLogger {
public:
    void debug(const std::string& msg) override { m_logs.push_back("DEBUG: " + msg); }
    void info(const std::string& msg) override { m_logs.push_back("INFO: " + msg); }
    void warn(const std::string& msg) override { m_logs.push_back("WARN: " + msg); }
    void error(const std::string& msg) override { m_logs.push_back("ERROR: " + msg); }
    
    const std::vector<std::string>& logs() const { return m_logs; }
    void clear() { m_logs.clear(); }
    
private:
    std::vector<std::string> m_logs;
};

class MockMetrics : public IConfigMetrics {
public:
    void incrementReloadSuccess() override { m_success_count++; }
    void incrementReloadFailure() override { m_failure_count++; }
    
    int successCount() const { return m_success_count; }
    int failureCount() const { return m_failure_count; }
    
private:
    int m_success_count{0};
    int m_failure_count{0};
};

class MockConfigSource : public IConfigSource {
public:
    std::string fetchConfig() override {
        if (m_should_fail) throw std::runtime_error("Config unavailable");
        return m_config;
    }
    
    void watchForChanges(ChangeCallback callback) override { m_callback = callback; }
    void start() override {}
    void stop() override {}
    
    void setConfig(const std::string& config) { m_config = config; }
    void setShouldFail(bool fail) { m_should_fail = fail; }
    void triggerChange(const std::string& new_config) {
        m_config = new_config;
        if (m_callback) m_callback(new_config);
    }
    
private:
    std::string m_config;
    ChangeCallback m_callback;
    bool m_should_fail{false};
};

class ConfigProviderDITest : public ::testing::Test {
protected:
    std::string getValidConfig() {
        return R"({
            "version": 1,
            "bootstrap": {
                "server": {"address": "127.0.0.1", "port": 8080},
                "threading": {"worker_threads": 4, "io_service_threads": 2},
                "database": {"mongodb_uri": "mongodb://localhost:27017", "redis_uri": "redis://localhost:6379"},
                "service": {"name": "test", "environment": "test"}
            },
            "operational": {"logging": {"level": "DEBUG", "format": "json", "enable_access_logs": true}},
            "runtime": {"rate_limiting": {"global_rps_limit": 100000, "per_user_rps_limit": 1000, "burst_size": 5000}}
        })";
    }
};

TEST_F(ConfigProviderDITest, CreatesWithInjectedLogger) {
    auto source = std::make_unique<MockConfigSource>();
    source->setConfig(getValidConfig());
    auto parser = std::make_unique<JsonConfigParser>();
    auto logger = std::make_shared<MockLogger>();
    
    auto provider = ConfigProvider::create(
        std::move(source),
        std::move(parser), 
        logger,
        nullptr  // No metrics
    );
    
    ASSERT_TRUE(provider.is_ok());
    EXPECT_FALSE(logger->logs().empty());
    EXPECT_TRUE(std::any_of(logger->logs().begin(), logger->logs().end(),
        [](const std::string& log) { return log.find("Loading initial configuration") != std::string::npos; }
    ));
}

TEST_F(ConfigProviderDITest, CreatesWithInjectedMetrics) {
    auto source_ptr = std::make_unique<MockConfigSource>();
    auto source_raw = source_ptr.get();
    source_raw->setConfig(getValidConfig());
    
    auto parser = std::make_unique<JsonConfigParser>();
    auto metrics = std::make_shared<MockMetrics>();
    
    auto provider = ConfigProvider::create(
        std::move(source_ptr),
        std::move(parser),
        nullptr,  // No logger
        metrics
    );
    
    ASSERT_TRUE(provider.is_ok());
    
    // Trigger a config change
    provider.value().start();
    source_raw->triggerChange(getValidConfig());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    provider.value().stop();
    
    EXPECT_EQ(metrics->successCount(), 1);
    EXPECT_EQ(metrics->failureCount(), 0);
}

TEST_F(ConfigProviderDITest, UsesDefaultsWhenNotProvided) {
    auto source = std::make_unique<MockConfigSource>();
    source->setConfig(getValidConfig());
    auto parser = std::make_unique<JsonConfigParser>();
    
    auto provider = ConfigProvider::create(
        std::move(source),
        std::move(parser)
        // No logger, no metrics - should use defaults
    );
    
    ASSERT_TRUE(provider.is_ok());
}

TEST_F(ConfigProviderDITest, ReturnsNulloptOnInitialLoadFailure) {
    auto source = std::make_unique<MockConfigSource>();
    source->setShouldFail(true);
    auto parser = std::make_unique<JsonConfigParser>();
    auto logger = std::make_shared<MockLogger>();
    
    auto provider = ConfigProvider::create(
        std::move(source),
        std::move(parser),
        logger,
        nullptr
    );
    
    EXPECT_FALSE(provider.is_ok());
    EXPECT_TRUE(std::any_of(logger->logs().begin(), logger->logs().end(),
        [](const std::string& log) { return log.find("ERROR") != std::string::npos; }
    ));
}

TEST_F(ConfigProviderDITest, LogsReloadFailureWithInjectedLogger) {
    auto source_ptr = std::make_unique<MockConfigSource>();
    auto source_raw = source_ptr.get();
    source_raw->setConfig(getValidConfig());
    
    auto parser = std::make_unique<JsonConfigParser>();
    auto logger = std::make_shared<MockLogger>();
    auto metrics = std::make_shared<MockMetrics>();
    
    auto provider = ConfigProvider::create(
        std::move(source_ptr),
        std::move(parser),
        logger,
        metrics
    );
    
    ASSERT_TRUE(provider.is_ok());
    provider.value().start();
    
    logger->clear();
    
    source_raw->triggerChange("{ invalid json }");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    provider.value().stop();
    
    EXPECT_EQ(metrics->failureCount(), 1);
    EXPECT_TRUE(std::any_of(logger->logs().begin(), logger->logs().end(),
        [](const std::string& log) { return log.find("ERROR") != std::string::npos && log.find("reload failed") != std::string::npos; }
    ));
}

}
