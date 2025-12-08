#include "ResponseHandle.h"
#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>
#include <thread>
#include <chrono>

using namespace http2server;
using namespace std::chrono_literals;

class ResponseHandleTest : public ::testing::Test {
protected:
    boost::asio::io_context io_ctx;
    int captured_status = -1;
    std::map<std::string, std::string> captured_headers;
    std::string captured_body;
    bool send_called = false;
    
    void SetUp() override {
        captured_status = -1;
        captured_headers.clear();
        captured_body.clear();
        send_called = false;
    }
    
    auto make_send_fn() {
        return [this](int status, std::map<std::string, std::string> headers, std::string body) {
            captured_status = status;
            captured_headers = std::move(headers);
            captured_body = std::move(body);
            send_called = true;
        };
    }
};

TEST_F(ResponseHandleTest, SendSuccess) {
    // Create handle
    auto handle = std::make_shared<ResponseHandle>(make_send_fn(), io_ctx);
    
    // Simulate worker thread sending response
    handle->send(200, {{"Content-Type", "text/plain"}}, "test response");
    
    // Run io_context to process posted task
    io_ctx.run();
    
    // Verify send was called
    EXPECT_TRUE(send_called);
    EXPECT_EQ(captured_status, 200);
    EXPECT_EQ(captured_headers["Content-Type"], "text/plain");
    EXPECT_EQ(captured_body, "test response");
}

TEST_F(ResponseHandleTest, SendAfterClose) {
    // Create handle
    auto handle = std::make_shared<ResponseHandle>(make_send_fn(), io_ctx);
    
    // Simulate stream closure
    handle->mark_closed();
    EXPECT_FALSE(handle->is_alive());
    
    // Worker tries to send after closure
    handle->send(200, {}, "should be dropped");
    
    // Run io_context
    io_ctx.run();
    
    // Verify send was NOT called (response dropped)
    EXPECT_FALSE(send_called);
    EXPECT_TRUE(captured_body.empty());
}

TEST_F(ResponseHandleTest, CloseAfterSendPosted) {
    // Create handle
    auto handle = std::make_shared<ResponseHandle>(make_send_fn(), io_ctx);
    
    // Worker posts send (but io_context not run yet)
    handle->send(200, {}, "test data");
    
    // Stream closes before io_context runs
    handle->mark_closed();
    
    // Run io_context - send was posted before closure
    io_ctx.run();
    
    // The posted lambda should still execute but check alive flag
    // Since closure happened after post, the data should be dropped
    EXPECT_FALSE(send_called);
}

TEST_F(ResponseHandleTest, WeakPtrExpiration) {
    // Create handle with strong reference
    auto handle = std::make_shared<ResponseHandle>(make_send_fn(), io_ctx);
    
    // Worker holds weak reference
    std::weak_ptr<ResponseHandle> weak_handle = handle;
    
    // Worker sends
    if (auto h = weak_handle.lock()) {
        h->send(200, {}, "test");
    }
    
    // Run io_context to process posted task (this keeps handle alive)
    io_ctx.run();
    io_ctx.restart();  // Reset for potential future use
    
    // Now destroy handle (simulating stream destruction)
    handle.reset();
    
    // Weak ptr should now be expired
    EXPECT_TRUE(weak_handle.expired());
    
    // Try to send again (should fail to lock)
    if (auto h = weak_handle.lock()) {
        FAIL() << "weak_ptr should have expired";
    } else {
        // Expected: cannot lock, so cannot send
        SUCCEED();
    }
}

TEST_F(ResponseHandleTest, IsAliveInitiallyTrue) {
    auto handle = std::make_shared<ResponseHandle>(make_send_fn(), io_ctx);
    EXPECT_TRUE(handle->is_alive());
}

TEST_F(ResponseHandleTest, IsAliveFalseAfterClose) {
    auto handle = std::make_shared<ResponseHandle>(make_send_fn(), io_ctx);
    handle->mark_closed();
    EXPECT_FALSE(handle->is_alive());
}

TEST_F(ResponseHandleTest, ConcurrentSends) {
    // Test multiple worker threads sending through same handle
    auto handle = std::make_shared<ResponseHandle>(
        [](int, std::map<std::string, std::string>, std::string) { /* no-op for this test */ }, 
        io_ctx
    );
    
    constexpr int NUM_THREADS = 4;
    constexpr int SENDS_PER_THREAD = 10;
    
    std::vector<std::thread> workers;
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        workers.emplace_back([handle, i]() {
            for (int j = 0; j < SENDS_PER_THREAD; ++j) {
                if (handle->is_alive()) {
                    handle->send(200, {}, "data_" + std::to_string(i) + "_" + std::to_string(j));
                }
                std::this_thread::sleep_for(1ms);
            }
        });
    }
    
    // Run io_context in separate thread
    std::thread io_thread([this]() {
        io_ctx.run();
    });
    
    // Wait for workers
    for (auto& t : workers) {
        t.join();
    }
    
    // Stop io_context and wait
    io_ctx.stop();
    io_thread.join();
    
    // No crashes = success
    SUCCEED();
}
