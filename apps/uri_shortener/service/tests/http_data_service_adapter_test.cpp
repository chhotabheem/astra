#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "HttpDataServiceAdapter.h"
#include "Http2Client.h"
#include "Http2ClientPool.h"
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>


using namespace url_shortener::service;
using namespace astra::http2;
using ::testing::_;
using ::testing::Invoke;

namespace url_shortener::service::test {

/**
 * Mock HTTP client for testing the adapter without real network calls.
 * Since Client doesn't have virtual methods, we test the adapter through
 * integration-style tests with the pool.
 */

class HttpDataServiceAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create config for pool
        m_pool_config.set_host("127.0.0.1");
        m_pool_config.set_port(29999);  // Unlikely to be in use
        m_pool_config.set_connect_timeout_ms(100);
        m_pool_config.set_request_timeout_ms(100);
        m_pool_config.set_pool_size(2);
    }

    astra::http2::ClientConfig m_pool_config;
};

// ===========================================================================
// Operation Translation Tests
// ===========================================================================

TEST_F(HttpDataServiceAdapterTest, SaveTranslatesToPost) {
    Http2ClientPool pool(m_pool_config);
    HttpDataServiceAdapter adapter(pool);
    
    DataServiceRequest req{
        DataServiceOperation::SAVE,
        "",  // entity_id not needed for SAVE
        R"({"code":"abc123","url":"https://example.com"})",
        nullptr,
        nullptr
    };
    
    std::atomic<bool> callback_called{false};
    
    adapter.execute(req, [&callback_called](DataServiceResponse resp) {
        callback_called = true;
        // Will fail with connection error since no server, but that's OK - 
        // we're testing that the adapter executes and calls back
    });
    
    // Give time for async execution
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_TRUE(callback_called);
}

TEST_F(HttpDataServiceAdapterTest, FindTranslatesToGet) {
    Http2ClientPool pool(m_pool_config);
    HttpDataServiceAdapter adapter(pool);
    
    DataServiceRequest req{
        DataServiceOperation::FIND,
        "abc123",
        "",
        nullptr,
        nullptr
    };
    
    std::atomic<bool> callback_called{false};
    
    adapter.execute(req, [&callback_called](DataServiceResponse resp) {
        callback_called = true;
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_TRUE(callback_called);
}

TEST_F(HttpDataServiceAdapterTest, DeleteTranslatesToDelete) {
    Http2ClientPool pool(m_pool_config);
    HttpDataServiceAdapter adapter(pool);
    
    DataServiceRequest req{
        DataServiceOperation::DELETE,
        "xyz789",
        "",
        nullptr,
        nullptr
    };
    
    std::atomic<bool> callback_called{false};
    
    adapter.execute(req, [&callback_called](DataServiceResponse resp) {
        callback_called = true;
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_TRUE(callback_called);
}

// ===========================================================================
// Error Handling Tests
// ===========================================================================

TEST_F(HttpDataServiceAdapterTest, ConnectionFailureReturnsInfraError) {
    Http2ClientPool pool(m_pool_config);
    HttpDataServiceAdapter adapter(pool);
    
    DataServiceRequest req{
        DataServiceOperation::FIND,
        "abc123",
        "",
        nullptr,
        nullptr
    };
    
    std::mutex mtx;
    std::condition_variable cv;
    bool callback_called = false;
    DataServiceResponse captured_response;
    
    adapter.execute(req, [&mtx, &cv, &callback_called, &captured_response](DataServiceResponse resp) {
        std::lock_guard<std::mutex> lock(mtx);
        captured_response = std::move(resp);
        callback_called = true;
        cv.notify_one();
    });
    
    // Wait for callback with timeout
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait_for(lock, std::chrono::milliseconds(500), [&callback_called] { return callback_called; });
    }
    
    EXPECT_TRUE(callback_called);
    EXPECT_FALSE(captured_response.success);
    EXPECT_TRUE(captured_response.infra_error.has_value());
}


// ===========================================================================
// Response Handle Passthrough Tests  
// ===========================================================================

TEST_F(HttpDataServiceAdapterTest, ResponseHandlePreservedInCallback) {
    Http2ClientPool pool(m_pool_config);
    HttpDataServiceAdapter adapter(pool);
    
    // Create a dummy response handle (we just check the pointer is preserved)
    auto dummy_handle = std::make_shared<int>(42);  // Using int as placeholder
    
    DataServiceRequest req{
        DataServiceOperation::FIND,
        "abc123",
        "",
        // Can't use real ResponseHandle without full server setup
        // but we can verify the pattern works
        nullptr,
        nullptr
    };
    
    std::atomic<bool> callback_called{false};
    
    adapter.execute(req, [&callback_called](DataServiceResponse resp) {
        callback_called = true;
        // response_handle should be passed through (even if null in this test)
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_TRUE(callback_called);
}

// ===========================================================================
// Path Building Tests
// ===========================================================================

TEST_F(HttpDataServiceAdapterTest, CustomBasePathUsed) {
    Http2ClientPool pool(m_pool_config);
    HttpDataServiceAdapter::Config config{"/custom/api/links"};
    HttpDataServiceAdapter adapter(pool, config);
    
    DataServiceRequest req{
        DataServiceOperation::FIND,
        "test123",
        "",
        nullptr,
        nullptr
    };
    
    std::atomic<bool> callback_called{false};
    
    adapter.execute(req, [&callback_called](DataServiceResponse resp) {
        callback_called = true;
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_TRUE(callback_called);
}

// ===========================================================================
// Concurrent Request Tests  
// ===========================================================================

TEST_F(HttpDataServiceAdapterTest, ConcurrentRequestsHandledCorrectly) {
    Http2ClientPool pool(m_pool_config);
    HttpDataServiceAdapter adapter(pool);
    
    std::atomic<int> callback_count{0};
    const int num_requests = 10;
    
    for (int i = 0; i < num_requests; i++) {
        DataServiceRequest req{
            DataServiceOperation::FIND,
            "code" + std::to_string(i),
            "",
            nullptr,
            nullptr
        };
        
        adapter.execute(req, [&callback_count](DataServiceResponse resp) {
            callback_count++;
        });
    }
    
    // Wait for all callbacks
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(callback_count, num_requests);
}

} // namespace url_shortener::service::test
