#include <gtest/gtest.h>
#include "ConfigProvider.h"
#include "IConfigSource.h"
#include "parsers/JsonConfigParser.h"
#include "observability/IConfigLogger.h"
#include "observability/IConfigMetrics.h"
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <algorithm>

namespace config {

// Mock classes for testing
class MockLogger : public IConfigLogger {
public:
    void debug(const std::string&) override {}
    void info(const std::string&) override {}
    void warn(const std::string&) override {}
    void error(const std::string& msg) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_errors.push_back(msg);
    }
    
    std::vector<std::string> getErrors() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_errors;
    }
    
private:
    mutable std::mutex m_mutex;
    std::vector<std::string> m_errors;
};

class MockMetrics : public IConfigMetrics {
public:
    void incrementReloadSuccess() override {
        m_success_count.fetch_add(1, std::memory_order_relaxed);
    }
    
    void incrementReloadFailure() override {
        m_failure_count.fetch_add(1, std::memory_order_relaxed);
    }
    
    int successCount() const { return m_success_count.load(); }
    int failureCount() const { return m_failure_count.load(); }
    
private:
    std::atomic<int> m_success_count{0};
    std::atomic<int> m_failure_count{0};
};

class MockConfigSource : public IConfigSource {
public:
    std::string fetchConfig() override {
        if (m_should_fail) throw std::runtime_error("Fetch failed");
        return m_config;
    }
    
    void watchForChanges(ChangeCallback callback) override {
        m_callback = callback;
    }
    
    void start() override { m_started = true; }
    void stop() override { m_started = false; }
    
    void setConfig(const std::string& config) { m_config = config; }
    void setShouldFail(bool fail) { m_should_fail = fail; }
    void triggerChange(const std::string& config) {
        m_config = config;
        if (m_callback) m_callback(config);
    }
    
private:
    std::string m_config;
    ChangeCallback m_callback;
    bool m_started{false};
    bool m_should_fail{false};
};

class ConfigProviderEdgeCasesTest : public ::testing::Test {
protected:
    std::string validConfig() {
        return R"({
            "version": 1,
            "bootstrap": {
                "server": {"address": "127.0.0.1", "port": 8080},
                "threading": {"worker_threads": 4, "io_service_threads": 2},
                "database": {"mongodb_uri": "mongodb://localhost", "redis_uri": "redis://localhost"},
                "service": {"name": "test", "environment": "test"}
            },
            "operational": {"logging": {"level": "DEBUG", "format": "json", "enable_access_logs": true}},
            "runtime": {"rate_limiting": {"global_rps_limit": 100000, "per_user_rps_limit": 1000, "burst_size": 5000}}
        })";
    }
};

// Test 1: Rapid sequential reloads
TEST_F(ConfigProviderEdgeCasesTest, RapidSequentialReloads) {
    auto source_ptr = std::make_unique<MockConfigSource>();
    auto source_raw = source_ptr.get();
    source_raw->setConfig(validConfig());
    
    auto parser = std::make_unique<JsonConfigParser>();
    auto metrics = std::make_shared<MockMetrics>();
    
    auto provider = ConfigProvider::create(std::move(source_ptr), std::move(parser), nullptr, metrics);
    ASSERT_TRUE(provider.is_ok());
    provider.value().start();
    
    // Trigger 10 rapid reloads
    for (int i = 0; i < 10; i++) {
        source_raw->triggerChange(validConfig());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    provider.value().stop();
    
    // All should succeed
    EXPECT_EQ(metrics->successCount(), 10);
    EXPECT_EQ(metrics->failureCount(), 0);
}

// Test 2: Callback throws exception
TEST_F(ConfigProviderEdgeCasesTest, CallbackThrowsException) {
    auto source_ptr = std::make_unique<MockConfigSource>();
    auto source_raw = source_ptr.get();
    source_raw->setConfig(validConfig());
    
    auto parser = std::make_unique<JsonConfigParser>();
    auto logger = std::make_shared<MockLogger>();
    
    auto provider = ConfigProvider::create(std::move(source_ptr), std::move(parser), logger, nullptr);
    ASSERT_TRUE(provider.is_ok());
    
    bool callback1_invoked = false;
    bool callback2_invoked = false;
    
    // First callback throws
    provider.value().onUpdate([&](const Config&) {
        callback1_invoked = true;
        throw std::runtime_error("Callback intentionally failed");
    });
    
    // Second callback should still run
    provider.value().onUpdate([&](const Config&) {
        callback2_invoked = true;
    });
    
    provider.value().start();
    source_raw->triggerChange(validConfig());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    provider.value().stop();
    
    // Both callbacks should have been invoked despite exception
    EXPECT_TRUE(callback1_invoked);
    EXPECT_TRUE(callback2_invoked);
    
    // Logger should have captured the error
    auto errors = logger->getErrors();
    EXPECT_FALSE(errors.empty());
    EXPECT_TRUE(std::any_of(errors.begin(), errors.end(),
        [](const std::string& err) { return err.find("Callback failed") != std::string::npos; }
    ));
}

// Test 3: Malformed JSON during reload
TEST_F(ConfigProviderEdgeCasesTest, MalformedJsonDuringReload) {
    auto source_ptr = std::make_unique<MockConfigSource>();
    auto source_raw = source_ptr.get();
    source_raw->setConfig(validConfig());
    
    auto parser = std::make_unique<JsonConfigParser>();
    auto metrics = std::make_shared<MockMetrics>();
    
    auto result = ConfigProvider::create(std::move(source_ptr), std::move(parser), nullptr, metrics);
    ASSERT_TRUE(result.is_ok());
    auto provider = std::move(result.value());
    provider.start();
    
    auto initial_config = provider.get();
    
    // Trigger with malformed JSON
    source_raw->triggerChange("{{{invalid}}}");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Config should remain unchanged
    EXPECT_EQ(provider.get(), initial_config);
    EXPECT_EQ(metrics->failureCount(), 1);
    
    provider.stop();
}

// Test 4: Empty config file
TEST_F(ConfigProviderEdgeCasesTest, EmptyConfigFile) {
    auto source = std::make_unique<MockConfigSource>();
    source->setConfig("");
    
    auto parser = std::make_unique<JsonConfigParser>();
    
    auto result = ConfigProvider::create(std::move(source), std::move(parser));
    
    // Should fail to create
    EXPECT_TRUE(result.is_err());
}

// Test 5: Invalid version number
TEST_F(ConfigProviderEdgeCasesTest, InvalidVersionNumber) {
    auto source = std::make_unique<MockConfigSource>();
    source->setConfig(R"({
        "version": 999,
        "bootstrap": {
            "server": {"address": "127.0.0.1", "port": 8080},
            "threading": {"worker_threads": 4, "io_service_threads": 2},
            "database": {"mongodb_uri": "mongodb://localhost", "redis_uri": "redis://localhost"},
            "service": {"name": "test", "environment": "test"}
        },
        "operational": {"logging": {"level": "DEBUG", "format": "json", "enable_access_logs": true}},
        "runtime": {"rate_limiting": {"global_rps_limit": 100000, "per_user_rps_limit": 1000, "burst_size": 5000}}
    })");
    
    auto parser = std::make_unique<JsonConfigParser>();
    auto logger = std::make_shared<MockLogger>();
    
    auto result = ConfigProvider::create(std::move(source), std::move(parser), logger, nullptr);
    
    EXPECT_TRUE(result.is_err());
    
    auto errors = logger->getErrors();
    EXPECT_TRUE(std::any_of(errors.begin(), errors.end(),
        [](const std::string& err) { return err.find("version") != std::string::npos; }
    ));
}

// Test 6: Concurrent get() calls during reload
TEST_F(ConfigProviderEdgeCasesTest, ConcurrentGetDuringReload) {
    auto source_ptr = std::make_unique<MockConfigSource>();
    auto source_raw = source_ptr.get();
    source_raw->setConfig(validConfig());
    
    auto parser = std::make_unique<JsonConfigParser>();
    
    auto result = ConfigProvider::create(std::move(source_ptr), std::move(parser));
    ASSERT_TRUE(result.is_ok());
    auto provider = std::move(result.value());
    provider.start();
    
    std::atomic<bool> stop{false};
    std::atomic<int> read_count{0};
    
    // Reader thread - continuously calls get()
    std::thread reader([&]() {
        while (!stop.load()) {
            auto config = provider.get();
            EXPECT_NE(config, nullptr);
            read_count.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });
    
    // Trigger reloads while reader is running
    for (int i = 0; i < 5; i++) {
        source_raw->triggerChange(validConfig());
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    stop.store(true);
    reader.join();
    provider.stop();
    
    // Should have done many reads without crashing
    EXPECT_GT(read_count.load(), 100);
}

// Test 7: Metrics actually increment
TEST_F(ConfigProviderEdgeCasesTest, MetricsActuallyIncrement) {
    auto source_ptr = std::make_unique<MockConfigSource>();
    auto source_raw = source_ptr.get();
    source_raw->setConfig(validConfig());
    
    auto parser = std::make_unique<JsonConfigParser>();
    auto metrics = std::make_shared<MockMetrics>();
    
    auto result = ConfigProvider::create(std::move(source_ptr), std::move(parser), nullptr, metrics);
    ASSERT_TRUE(result.is_ok());
    auto provider = std::move(result.value());
    provider.start();
    
    EXPECT_EQ(metrics->successCount(), 0);
    EXPECT_EQ(metrics->failureCount(), 0);
    
    // Successful reload
    source_raw->triggerChange(validConfig());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    EXPECT_EQ(metrics->successCount(), 1);
    EXPECT_EQ(metrics->failureCount(), 0);
    
    // Failed reload
    source_raw->triggerChange("invalid");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    EXPECT_EQ(metrics->successCount(), 1);
    EXPECT_EQ(metrics->failureCount(), 1);
    
    provider.stop();
}

// Test 8: Missing required fields
TEST_F(ConfigProviderEdgeCasesTest, MissingRequiredFields) {
    auto source = std::make_unique<MockConfigSource>();
    source->setConfig(R"({
        "version": 1,
        "bootstrap": {
            "server": {"address": "127.0.0.1"}
        }
    })");
    
    auto parser = std::make_unique<JsonConfigParser>();
    
    auto result = ConfigProvider::create(std::move(source), std::move(parser));
    
    // Should fail validation due to missing port
    EXPECT_TRUE(result.is_err());
}

}
