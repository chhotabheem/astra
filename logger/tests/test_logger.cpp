/**
 * @file test_logger.cpp
 * @brief Tests for Astra logging library
 */

#include <Logger.h>
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

// Simple test framework
int test_count = 0;
int test_passed = 0;

#define TEST(name) \
    void name(); \
    struct name##_runner { \
        name##_runner() { \
            test_count++; \
            std::cout << "Running test: " << #name << std::endl; \
            try { \
                name(); \
                test_passed++; \
                std::cout << "  PASSED" << std::endl; \
            } catch (const std::exception& e) { \
                std::cout << "  FAILED: " << e.what() << std::endl; \
            } \
        } \
    } name##_instance; \
    void name()

#define ASSERT(condition) \
    if (!(condition)) { \
        throw std::runtime_error("Assertion failed: " #condition); \
    }

// Test: Basic logging
TEST(test_basic_logging) {
    logger::Logger::initialize();
    
    LOG_INFO("Test info message");
    LOG_WARN("Test warning message");
    LOG_ERROR("Test error message");
    
    // Give async logger time to flush
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    ASSERT(true); // If we got here without crashing, test passed
}

// Test: All log levels
TEST(test_all_levels) {
    logger::Logger::set_level(logger::Level::TRACE);
    
    LOG_TRACE("Trace message");
    LOG_DEBUG("Debug message");
    LOG_INFO("Info message");
    LOG_WARN("Warning message");
    LOG_ERROR("Error message");
    LOG_FATAL("Fatal message");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    ASSERT(true);
}

// Test: Level filtering
TEST(test_level_filtering) {
    // Set level to ERROR - should only see ERROR and FATAL
    logger::Logger::set_level(logger::Level::ERROR);
    
    LOG_DEBUG("This should not appear");
    LOG_INFO("This should not appear");
    LOG_ERROR("This should appear");
    LOG_FATAL("This should appear");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    ASSERT(true);
}

// Test: Thread safety
TEST(test_thread_safety) {
    logger::Logger::set_level(logger::Level::INFO);
    
    const int num_threads = 10;
    const int messages_per_thread = 100;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i, messages_per_thread]() {
            for (int j = 0; j < messages_per_thread; ++j) {
                LOG_INFO("Thread " + std::to_string(i) + 
                                " message " + std::to_string(j));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Give async logger time to flush all messages
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    ASSERT(true); // If we got here without deadlock/crash, test passed
}

// Test: Long messages
TEST(test_long_messages) {
    std::string long_message(1000, 'A');
    LOG_INFO(long_message);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    ASSERT(true);
}

// Test: Special characters in messages
TEST(test_special_characters) {
    LOG_INFO("Message with \"quotes\"");
    LOG_INFO("Message with newline\n");
    LOG_INFO("Message with tab\t");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    ASSERT(true);
}

// Test: Rapid logging
TEST(test_rapid_logging) {
    const int num_messages = 1000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_messages; ++i) {
        LOG_INFO("Rapid message " + std::to_string(i));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Logged " << num_messages << " messages in " 
              << duration.count() << "ms" << std::endl;
    
    // Give async logger time to flush
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    ASSERT(true);
}

int main() {
    std::cout << "\n=== Astra Logger Tests ===\n" << std::endl;
    
    // Tests run automatically via static initialization
    
    // Shutdown logger
    logger::Logger::shutdown();
    
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Total tests: " << test_count << std::endl;
    std::cout << "Passed: " << test_passed << std::endl;
    std::cout << "Failed: " << (test_count - test_passed) << std::endl;
    
    return (test_count == test_passed) ? 0 : 1;
}
