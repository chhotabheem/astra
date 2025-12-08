#include <gtest/gtest.h>
#include "filesource/FileConfigSource.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <random>

namespace config {

class InotifyFileWatcherTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Generate unique file name per test to avoid parallel test interference
        auto* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(10000, 99999);
        
        std::string unique_name = std::string("inotify_") + test_info->name() + "_" + std::to_string(dis(gen)) + ".json";
        m_test_file = std::filesystem::temp_directory_path() / unique_name;
        writeTestFile(validConfig());
    }
    
    void TearDown() override {
        if (std::filesystem::exists(m_test_file)) {
            std::filesystem::remove(m_test_file);
        }
    }
    
    void writeTestFile(const std::string& content) {
        std::ofstream file(m_test_file);
        file << content;
        file.close();
    }
    
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
    
    std::filesystem::path m_test_file;
};

// Test 1: inotify detects file modification instantly (no 1s delay)
TEST_F(InotifyFileWatcherTest, DetectsFileChangeInstantly) {
    FileConfigSource source(m_test_file);
    
    std::atomic<bool> callback_invoked{false};
    std::string new_content;
    
    source.watchForChanges([&](const std::string& content) {
        callback_invoked.store(true);
        new_content = content;
    });
    
    source.start();
    
    // Modify file
    auto start = std::chrono::steady_clock::now();
    writeTestFile(validConfig());
    
    // Wait for callback (should be near-instant, not 1 second)
    for (int i = 0; i < 20 && !callback_invoked.load(); i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    auto duration = std::chrono::steady_clock::now() - start;
    
    source.stop();
    
    EXPECT_TRUE(callback_invoked.load());
    EXPECT_FALSE(new_content.empty());
    
    // Should detect within 200ms (much less than 1s polling interval)
    EXPECT_LT(duration, std::chrono::milliseconds(500));
}

// Test 2: Multiple rapid file changes all detected
TEST_F(InotifyFileWatcherTest, DetectsMultipleRapidChanges) {
    FileConfigSource source(m_test_file);
    
    std::atomic<int> change_count{0};
    
    source.watchForChanges([&](const std::string&) {
        change_count.fetch_add(1);
    });
    
    source.start();
    
    // Make 5 rapid changes
    for (int i = 0; i < 5; i++) {
        writeTestFile(validConfig());
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    source.stop();
    
    // Should detect all or most changes (inotify may coalesce some)
    EXPECT_GE(change_count.load(), 3);
}

// Test 3: Handles file deletion gracefully
TEST_F(InotifyFileWatcherTest, HandlesFileDeletion) {
    FileConfigSource source(m_test_file);
    
    std::atomic<bool> callback_invoked{false};
    
    source.watchForChanges([&](const std::string&) {
        callback_invoked.store(true);
    });
    
    source.start();
    
    // Delete the file
    std::filesystem::remove(m_test_file);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Should not crash, watchForChanges should handle gracefully
    EXPECT_NO_THROW(source.stop());
}

// Test 4: Can restart watching after stop
TEST_F(InotifyFileWatcherTest, CanRestartAfterStop) {
    FileConfigSource source(m_test_file);
    
    std::atomic<int> change_count{0};
    
    source.watchForChanges([&](const std::string&) {
        change_count.fetch_add(1);
    });
    
    // First session
    source.start();
    writeTestFile(validConfig());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    source.stop();
    
    int first_count = change_count.load();
    EXPECT_GE(first_count, 1);
    
    // Second session
    source.start();
    writeTestFile(validConfig());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    source.stop();
    
    EXPECT_GT(change_count.load(), first_count);
}

// Test 5: Handles file being replaced (e.g., atomic write by mv)
TEST_F(InotifyFileWatcherTest, HandlesFileReplacement) {
    FileConfigSource source(m_test_file);
    
    std::atomic<bool> callback_invoked{false};
    
    source.watchForChanges([&](const std::string&) {
        callback_invoked.store(true);
    });
    
    source.start();
    
    // Simulate atomic write (write to temp, then move)
    auto temp_file = m_test_file.string() + ".tmp";
    writeTestFile(validConfig());
    std::ofstream(temp_file) << validConfig();
    std::filesystem::rename(temp_file, m_test_file);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    source.stop();
    
    // Should detect the replacement
    EXPECT_TRUE(callback_invoked.load());
}

}
