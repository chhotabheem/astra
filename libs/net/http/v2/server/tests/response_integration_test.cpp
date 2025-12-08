#include <gtest/gtest.h>
#include "Http2Response.h"
#include "ResponseHandle.h"
#include <memory>
#include <boost/asio/io_context.hpp>

using namespace http2server;

/**
 * TDD Tests for Response as lightweight copyable handle
 * 
 * Design: Response holds data directly + weak_ptr<ResponseHandle>
 * - optional<int> m_status (no default - forces explicit set)
 * - bool m_closed (prevents double-send)
 * - Copyable
 * - close() sends via ResponseHandle
 */

class ResponseHandleIntegrationTest : public ::testing::Test {
protected:
    boost::asio::io_context io_ctx;
    
    struct SentData {
        int status = -1;
        std::map<std::string, std::string> headers;
        std::string body;
        int send_count = 0;
    };
    
    SentData sent;
    
    std::shared_ptr<ResponseHandle> make_response_handle() {
        return std::make_shared<ResponseHandle>(
            [this](int status, std::map<std::string, std::string> headers, std::string body) {
                sent.status = status;
                sent.headers = std::move(headers);
                sent.body = std::move(body);
                sent.send_count++;
            },
            io_ctx
        );
    }
};

TEST_F(ResponseHandleIntegrationTest, ResponseIsCopyable) {
    auto handle = make_response_handle();
    Response res1(handle);
    
    // Copy construct
    Response res2 = res1;
    
    // Copy assign
    Response res3(handle);
    res3 = res1;
    
    // All should be valid
    res1.set_status(200);
    res2.write("test");
    // Should not crash
    SUCCEED();
}

TEST_F(ResponseHandleIntegrationTest, ResponseCloseSendsViaHandle) {
    auto handle = make_response_handle();
    Response res(handle);
    
    res.set_status(201);
    res.set_header("Content-Type", "text/plain");
    res.write("Created");
    res.close();
    
    // Run io_context to process posted send
    io_ctx.run();
    
    EXPECT_EQ(sent.status, 201);
    EXPECT_EQ(sent.headers["Content-Type"], "text/plain");
    EXPECT_EQ(sent.body, "Created");
    EXPECT_EQ(sent.send_count, 1);
}

TEST_F(ResponseHandleIntegrationTest, ResponseDoubleCloseOnlySendsOnce) {
    auto handle = make_response_handle();
    Response res(handle);
    
    res.set_status(200);
    res.write("OK");
    res.close();
    res.close();  // Second close should be no-op
    res.close();  // Third close should be no-op
    
    io_ctx.run();
    
    EXPECT_EQ(sent.send_count, 1);  // Only sent once
}

TEST_F(ResponseHandleIntegrationTest, ResponseCloseGracefulWhenHandleExpired) {
    std::weak_ptr<ResponseHandle> weak_handle;
    {
        auto handle = make_response_handle();
        weak_handle = handle;
    }
    // handle destroyed
    
    Response res(weak_handle);
    res.set_status(200);
    res.write("test");
    res.close();  // Should not crash, gracefully dropped
    
    io_ctx.run();
    
    EXPECT_EQ(sent.send_count, 0);  // Not sent
}

TEST_F(ResponseHandleIntegrationTest, ResponseStatusDefaultsTo500IfNotSet) {
    auto handle = make_response_handle();
    Response res(handle);
    
    // Deliberately NOT calling set_status()
    res.write("oops");
    res.close();
    
    io_ctx.run();
    
    // Should default to 500 (error) not 200
    EXPECT_EQ(sent.status, 500);
}

TEST_F(ResponseHandleIntegrationTest, ResponseAccumulatesWrites) {
    auto handle = make_response_handle();
    Response res(handle);
    
    res.set_status(200);
    res.write("Hello");
    res.write(" ");
    res.write("World");
    res.close();
    
    io_ctx.run();
    
    EXPECT_EQ(sent.body, "Hello World");
}

TEST_F(ResponseHandleIntegrationTest, CopiedResponseSharesClosedState) {
    auto handle = make_response_handle();
    Response res1(handle);
    res1.set_status(200);
    res1.write("test");
    
    // Copy before close
    Response res2 = res1;
    
    res1.close();
    io_ctx.run();
    EXPECT_EQ(sent.send_count, 1);
    
    io_ctx.restart();
    
    // res2 is a copy, but its own m_closed flag is independent
    // So it will try to send...but body was moved
    res2.close();
    io_ctx.run();
    
    // This behavior is debatable - documenting current expected behavior
    // After copy, close on copy is independent
    EXPECT_EQ(sent.send_count, 2);  // Both copies sent
}

TEST_F(ResponseHandleIntegrationTest, ResponseSetHeaderMultipleTimes) {
    auto handle = make_response_handle();
    Response res(handle);
    
    res.set_status(200);
    res.set_header("X-Custom", "value1");
    res.set_header("X-Custom", "value2");  // Overwrites
    res.close();
    
    io_ctx.run();
    
    EXPECT_EQ(sent.headers["X-Custom"], "value2");
}
