#include <gtest/gtest.h>
#include "filesource/FileConfigSource.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <random>

namespace config {

class FileConfigSourceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Generate unique file name per test to avoid parallel test interference
        auto* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(10000, 99999);
        
        std::string unique_name = std::string(test_info->name()) + "_" + std::to_string(dis(gen)) + ".yaml";
        m_test_file = std::filesystem::temp_directory_path() / unique_name;
        writeTestConfig("initial_config_data");
    }
    
    void TearDown() override {
        if (std::filesystem::exists(m_test_file)) {
            std::filesystem::remove(m_test_file);
        }
    }
    
    void writeTestConfig(const std::string& content) {
        std::ofstream file(m_test_file);
        file << content;
        file.close();
    }
    
    std::filesystem::path m_test_file;
};

TEST_F(FileConfigSourceTest, FetchConfigReadsFileContent) {
    FileConfigSource source(m_test_file);
    
    std::string content = source.fetchConfig();
    
    EXPECT_EQ(content, "initial_config_data");
}

TEST_F(FileConfigSourceTest, FetchConfigThrowsIfFileDoesNotExist) {
    std::filesystem::path nonexistent = "/nonexistent/path/config.yaml";
    FileConfigSource source(nonexistent);
    
    EXPECT_THROW(source.fetchConfig(), std::runtime_error);
}

#include "IExecutor.h"

#include "IExecutor.h"
#include <thread>

class MockExecutor : public astra::execution::IExecutor {
public:
    void submit(std::function<void()> task) override {
        submit_count++;
        // Run in a thread to avoid blocking the test, just like ThreadExecutor
        std::thread([task = std::move(task)]() {
            task();
        }).detach();
    }
    
    int submit_count = 0;
};

TEST_F(FileConfigSourceTest, UsesInjectedExecutor) {
    auto executor = std::make_shared<MockExecutor>();
    auto source = std::make_unique<config::FileConfigSource>(m_test_file, executor);
    
    source->start();
    
    EXPECT_EQ(executor->submit_count, 1);
    
    // Stop will signal the loop (running in the detached thread) to exit
    // and wait for it to finish.
    source->stop();
}
TEST_F(FileConfigSourceTest, WatchForChangesDetectsFileModification) {
    FileConfigSource source(m_test_file);
    
    std::string received_content;
    bool callback_invoked = false;
    
    source.watchForChanges([&](const std::string& content) {
        received_content = content;
        callback_invoked = true;
    });
    
    source.start();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    writeTestConfig("updated_config_data");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    
    source.stop();
    
    EXPECT_TRUE(callback_invoked);
    EXPECT_EQ(received_content, "updated_config_data");
}

TEST_F(FileConfigSourceTest, StartStopLifecycle) {
    FileConfigSource source(m_test_file);
    
    source.watchForChanges([](const std::string&) {});
    
    EXPECT_NO_THROW(source.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_NO_THROW(source.stop());
}

TEST_F(FileConfigSourceTest, MultipleFileChangesDetected) {
    FileConfigSource source(m_test_file);
    
    int change_count = 0;
    
    source.watchForChanges([&](const std::string&) {
        change_count++;
    });
    
    source.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    writeTestConfig("change1");
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    
    writeTestConfig("change2");
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    
    source.stop();
    
    EXPECT_EQ(change_count, 2);
}

TEST_F(FileConfigSourceTest, NoCallbackWithoutChanges) {
    FileConfigSource source(m_test_file);
    
    bool callback_invoked = false;
    
    source.watchForChanges([&](const std::string&) {
        callback_invoked = true;
    });
    
    source.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    source.stop();
    
    EXPECT_FALSE(callback_invoked);
}

}
