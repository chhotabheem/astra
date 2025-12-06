#include <gtest/gtest.h>
#include "IConfigSource.h"
#include <thread>
#include <chrono>

namespace config {

class MockConfigSource : public IConfigSource {
public:
    std::string fetchConfig() override {
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
    
    bool isStarted() const { return m_started; }
    
private:
    std::string m_stored_config;
    ChangeCallback m_callback;
    bool m_started{false};
};

TEST(IConfigSourceTest, FetchConfigReturnsStoredValue) {
    MockConfigSource source;
    source.setConfig("test_config_data");
    
    EXPECT_EQ(source.fetchConfig(), "test_config_data");
}

TEST(IConfigSourceTest, WatchForChangesRegistersCallback) {
    MockConfigSource source;
    std::string received_config;
    
    source.watchForChanges([&](const std::string& config) {
        received_config = config;
    });
    
    source.triggerChange("new_config");
    
    EXPECT_EQ(received_config, "new_config");
}

TEST(IConfigSourceTest, StartStopLifecycle) {
    MockConfigSource source;
    
    EXPECT_FALSE(source.isStarted());
    
    source.start();
    EXPECT_TRUE(source.isStarted());
    
    source.stop();
    EXPECT_FALSE(source.isStarted());
}

TEST(IConfigSourceTest, MultipleCallbackInvocations) {
    MockConfigSource source;
    int call_count = 0;
    
    source.watchForChanges([&](const std::string&) {
        call_count++;
    });
    
    source.triggerChange("config1");
    source.triggerChange("config2");
    source.triggerChange("config3");
    
    EXPECT_EQ(call_count, 3);
}

}
