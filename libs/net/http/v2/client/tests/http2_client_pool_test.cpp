#include <gtest/gtest.h>
#include "Http2ClientPool.h"
#include <thread>
#include <set>
#include <atomic>

namespace astra::http2 {
namespace test {

/**
 * Note: These tests create actual Client instances which attempt to connect.
 * The connection will fail (no server running), but we can still test:
 * 1. Pool construction creates correct number of clients
 * 2. Round-robin selection works correctly
 * 3. Thread-safety of get() method
 * 
 * The Client constructor doesn't throw on connection failure, it just logs.
 */
class Http2ClientPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a config that won't cause immediate crashes
        // Connection will fail gracefully
        m_config.set_host("127.0.0.1");
        m_config.set_port(19999);  // Unlikely to be in use
        m_config.set_connect_timeout_ms(100);
        m_config.set_request_timeout_ms(100);
        m_config.set_pool_size(3);
    }

    ClientConfig m_config;
};

// ===========================================================================
// Construction Tests
// ===========================================================================

TEST_F(Http2ClientPoolTest, ConstructionCreatesCorrectNumberOfClients) {
    m_config.set_pool_size(5);
    Http2ClientPool pool(m_config);
    
    EXPECT_EQ(pool.size(), 5);
}

TEST_F(Http2ClientPoolTest, ConstructionWithZeroPoolSizeDefaultsToOne) {
    m_config.set_pool_size(0);
    Http2ClientPool pool(m_config);
    
    EXPECT_GE(pool.size(), 1);
}

TEST_F(Http2ClientPoolTest, ConstructionWithPoolSizeOne) {
    m_config.set_pool_size(1);
    Http2ClientPool pool(m_config);
    
    EXPECT_EQ(pool.size(), 1);
}

// ===========================================================================
// Round-Robin Distribution Tests
// ===========================================================================

TEST_F(Http2ClientPoolTest, GetReturnsClientReference) {
    m_config.set_pool_size(2);
    Http2ClientPool pool(m_config);
    
    Client& client = pool.get();
    // Should not throw, should return valid reference
    (void)client;
    SUCCEED();
}

TEST_F(Http2ClientPoolTest, RoundRobinDistributesAcrossAllClients) {
    m_config.set_pool_size(3);
    Http2ClientPool pool(m_config);
    
    std::set<Client*> unique_clients;
    
    // Get 9 times - should cycle through all 3 clients 3 times
    for (int i = 0; i < 9; i++) {
        unique_clients.insert(&pool.get());
    }
    
    EXPECT_EQ(unique_clients.size(), 3);
}

TEST_F(Http2ClientPoolTest, SingleClientPoolReturnsSameClient) {
    m_config.set_pool_size(1);
    Http2ClientPool pool(m_config);
    
    Client* first = &pool.get();
    Client* second = &pool.get();
    Client* third = &pool.get();
    
    EXPECT_EQ(first, second);
    EXPECT_EQ(second, third);
}

TEST_F(Http2ClientPoolTest, SequentialGetsCycleCorrectly) {
    m_config.set_pool_size(3);
    Http2ClientPool pool(m_config);
    
    // First cycle
    Client* c0 = &pool.get();
    Client* c1 = &pool.get();
    Client* c2 = &pool.get();
    
    // Second cycle - should return same clients in same order
    EXPECT_EQ(&pool.get(), c0);
    EXPECT_EQ(&pool.get(), c1);
    EXPECT_EQ(&pool.get(), c2);
}

// ===========================================================================
// Thread Safety Tests
// ===========================================================================

TEST_F(Http2ClientPoolTest, ConcurrentAccessDoesNotCrash) {
    m_config.set_pool_size(4);
    Http2ClientPool pool(m_config);
    
    std::atomic<int> success_count{0};
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 50; i++) {  // Reduced from 100 for faster tests
        threads.emplace_back([&pool, &success_count]() {
            try {
                Client& client = pool.get();
                (void)client;  // Suppress unused warning
                success_count++;
            } catch (...) {
                // Should not throw
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(success_count, 50);
}

// ===========================================================================
// Size Edge Cases
// ===========================================================================

TEST_F(Http2ClientPoolTest, SizeRemainsConstant) {
    m_config.set_pool_size(3);
    Http2ClientPool pool(m_config);
    
    EXPECT_EQ(pool.size(), 3);
    
    // Getting clients doesn't change size
    for (int i = 0; i < 10; i++) {
        pool.get();
    }
    
    EXPECT_EQ(pool.size(), 3);
}

} // namespace test
} // namespace astra::http2
