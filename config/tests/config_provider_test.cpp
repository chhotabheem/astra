#include <gtest/gtest.h>
#include "ConfigProvider.h"
#include "IConfigSource.h"
#include "parsers/JsonConfigParser.h"
#include <memory>
#include <thread>
#include <chrono>

namespace config {

class MockConfigSource : public IConfigSource {
public:
    std::string fetchConfig() override {
        if (m_should_fail) {
            throw std::runtime_error("Config source unavailable");
        }
        return m_stored_config;
    }
    
    void watchForChanges(ChangeCallback callback) override {
        m_callback = callback;
    }
    
    void start() override {
        m_started = true;
    }
    
    void stop() override {
        m_started = false;
    }
    
    void setConfig(const std::string& config) {
        m_stored_config = config;
    }
    
    void triggerChange(const std::string& new_config) {
        m_stored_config = new_config;
        if (m_callback) {
            m_callback(new_config);
        }
    }
    
    void setShouldFail(bool should_fail) {
        m_should_fail = should_fail;
    }
    
private:
    std::string m_stored_config;
    ChangeCallback m_callback;
    bool m_started{false};
    bool m_should_fail{false};
};

class ConfigProviderTest : public ::testing::Test {
protected:
    std::string getValidConfig() {
        return R"({
            "bootstrap": {
                "server": {"address": "127.0.0.1", "port": 8080},
                "threading": {"worker_threads": 4, "io_service_threads": 2},
                "database": {
                    "mongodb_uri": "mongodb://localhost:27017",
                    "redis_uri": "redis://localhost:6379"
                },
                "service": {"name": "test-service", "environment": "test"}
            },
            "operational": {
                "logging": {"level": "DEBUG", "format": "json", "enable_access_logs": true}
            },
            "runtime": {
                "rate_limiting": {"global_rps_limit": 100000, "per_user_rps_limit": 1000, "burst_size": 5000}
            }
        })";
    }
    
    std::string getUpdatedConfig() {
        return R"({
            "bootstrap": {
                "server": {"address": "127.0.0.1", "port": 9090},
                "threading": {"worker_threads": 8, "io_service_threads": 4},
                "database": {
                    "mongodb_uri": "mongodb://localhost:27017",
                    "redis_uri": "redis://localhost:6379"
                },
                "service": {"name": "test-service", "environment": "test"}
            },
            "operational": {
                "logging": {"level": "INFO", "format": "json", "enable_access_logs": false}
            },
            "runtime": {
                "rate_limiting": {"global_rps_limit": 200000, "per_user_rps_limit": 2000, "burst_size": 10000}
            }
        })";
    }
};

TEST_F(ConfigProviderTest, LoadsInitialConfigOnCreation) {
    auto source = std::make_unique<MockConfigSource>();
    source->setConfig(getValidConfig());
    
    auto parser = std::make_unique<JsonConfigParser>();
    
    auto result = ConfigProvider::create(std::move(source), std::move(parser));
    
    ASSERT_TRUE(result.is_ok());
    auto provider = std::move(result.value());
    auto config = provider.get();
    ASSERT_NE(config, nullptr);
    EXPECT_EQ(config->m_bootstrap.m_server.m_port, 8080);
    EXPECT_EQ(config->m_operational.m_logging.m_level, "DEBUG");
}

TEST_F(ConfigProviderTest, ThrowsIfInitialConfigFetchFails) {
    auto source = std::make_unique<MockConfigSource>();
    source->setShouldFail(true);
    
    auto parser = std::make_unique<JsonConfigParser>();
    
    auto result = ConfigProvider::create(std::move(source), std::move(parser));
    
    EXPECT_TRUE(result.is_err());
    EXPECT_FALSE(result.error().empty());
}

TEST_F(ConfigProviderTest, GetReturnsCurrentConfig) {
    auto source = std::make_unique<MockConfigSource>();
    source->setConfig(getValidConfig());
    
    auto parser = std::make_unique<JsonConfigParser>();
    
    auto result = ConfigProvider::create(std::move(source), std::move(parser));
    ASSERT_TRUE(result.is_ok());
    auto provider = std::move(result.value());
    
    auto config1 = provider.get();
    auto config2 = provider.get();
    
    EXPECT_EQ(config1, config2);
}

TEST_F(ConfigProviderTest, HotReloadUpdatesConfig) {
    auto source_ptr = std::make_unique<MockConfigSource>();
    auto source_raw = source_ptr.get();
    source_raw->setConfig(getValidConfig());
    
    auto parser = std::make_unique<JsonConfigParser>();
    
    auto result = ConfigProvider::create(std::move(source_ptr), std::move(parser));
    ASSERT_TRUE(result.is_ok());
    auto provider = std::move(result.value());
    provider.start();
    
    auto initial_config = provider.get();
    EXPECT_EQ(initial_config->m_bootstrap.m_server.m_port, 8080);
    
    source_raw->triggerChange(getUpdatedConfig());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto updated_config = provider.get();
    EXPECT_NE(initial_config, updated_config);
    EXPECT_EQ(updated_config->m_bootstrap.m_server.m_port, 9090);
    EXPECT_EQ(updated_config->m_operational.m_logging.m_level, "INFO");
    
    provider.stop();
}

TEST_F(ConfigProviderTest, CallbacksInvokedOnConfigUpdate) {
    auto source_ptr = std::make_unique<MockConfigSource>();
    auto source_raw = source_ptr.get();
    source_raw->setConfig(getValidConfig());
    
    auto parser = std::make_unique<JsonConfigParser>();
    
    auto result = ConfigProvider::create(std::move(source_ptr), std::move(parser));
    ASSERT_TRUE(result.is_ok());
    auto provider = std::move(result.value());
    
    bool callback_invoked = false;
    uint16_t new_port = 0;
    
    provider.onUpdate([&](const Config& config) {
        callback_invoked = true;
        new_port = config.m_bootstrap.m_server.m_port;
    });
    
    provider.start();
    
    source_raw->triggerChange(getUpdatedConfig());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    provider.stop();
    
    EXPECT_TRUE(callback_invoked);
    EXPECT_EQ(new_port, 9090);
}

TEST_F(ConfigProviderTest, MultipleCallbacksAllInvoked) {
    auto source_ptr = std::make_unique<MockConfigSource>();
    auto source_raw = source_ptr.get();
    source_raw->setConfig(getValidConfig());
    
    auto parser = std::make_unique<JsonConfigParser>();
    
    auto result = ConfigProvider::create(std::move(source_ptr), std::move(parser));
    ASSERT_TRUE(result.is_ok());
    auto provider = std::move(result.value());
    
    int callback1_count = 0;
    int callback2_count = 0;
    
    provider.onUpdate([&](const Config&) { callback1_count++; });
    provider.onUpdate([&](const Config&) { callback2_count++; });
    
    provider.start();
    
    source_raw->triggerChange(getUpdatedConfig());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    provider.stop();
    
    EXPECT_EQ(callback1_count, 1);
    EXPECT_EQ(callback2_count, 1);
}

TEST_F(ConfigProviderTest, InvalidConfigOnReloadKeepsOldConfig) {
    auto source_ptr = std::make_unique<MockConfigSource>();
    auto source_raw = source_ptr.get();
    source_raw->setConfig(getValidConfig());
    
    auto parser = std::make_unique<JsonConfigParser>();
    
    auto result = ConfigProvider::create(std::move(source_ptr), std::move(parser));
    ASSERT_TRUE(result.is_ok());
    auto provider = std::move(result.value());
    provider.start();
    
    auto initial_config = provider.get();
    
    source_raw->triggerChange("{ invalid json }");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto current_config = provider.get();
    
    EXPECT_EQ(initial_config, current_config);
    
    provider.stop();
}

TEST_F(ConfigProviderTest, StartStopLifecycle) {
    auto source = std::make_unique<MockConfigSource>();
    source->setConfig(getValidConfig());
    
    auto parser = std::make_unique<JsonConfigParser>();
    
    auto result = ConfigProvider::create(std::move(source), std::move(parser));
    ASSERT_TRUE(result.is_ok());
    auto provider = std::move(result.value());
    
    EXPECT_NO_THROW(provider.start());
    EXPECT_NO_THROW(provider.stop());
    EXPECT_NO_THROW(provider.start());
    EXPECT_NO_THROW(provider.stop());
}

}
