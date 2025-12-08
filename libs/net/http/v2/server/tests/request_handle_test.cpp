#include <gtest/gtest.h>
#include "Http2Request.h"
#include "RequestData.h"
#include <memory>

using namespace http2server;

/**
 * TDD Tests for Request as lightweight copyable handle
 * 
 * Design: Request holds weak_ptr<RequestData>
 * - Copyable (not move-only)
 * - Each method locks weak_ptr, returns empty if expired
 * - Graceful behavior when stream closed
 */

class RequestHandleTest : public ::testing::Test {
protected:
    std::shared_ptr<RequestData> make_request_data() {
        auto data = std::make_shared<RequestData>();
        data->method = "POST";
        data->path = "/api/shorten";
        data->body = R"({"url":"https://example.com"})";
        data->headers["content-type"] = "application/json";
        data->headers["accept"] = "application/json";
        data->path_params["id"] = "abc123";
        return data;
    }
};

TEST_F(RequestHandleTest, RequestIsCopyable) {
    auto data = make_request_data();
    Request req1(data);
    
    // Copy construct
    Request req2 = req1;
    
    // Copy assign
    Request req3(data);
    req3 = req1;
    
    // All should access same underlying data
    EXPECT_EQ(req1.method(), "POST");
    EXPECT_EQ(req2.method(), "POST");
    EXPECT_EQ(req3.method(), "POST");
}

TEST_F(RequestHandleTest, RequestMethodsWorkViaWeakPtrLock) {
    auto data = make_request_data();
    Request req(data);
    
    EXPECT_EQ(req.method(), "POST");
    EXPECT_EQ(req.path(), "/api/shorten");
    EXPECT_EQ(req.body(), R"({"url":"https://example.com"})");
    EXPECT_EQ(req.header("content-type"), "application/json");
    EXPECT_EQ(req.path_param("id"), "abc123");
}

TEST_F(RequestHandleTest, RequestReturnsEmptyWhenWeakPtrExpired) {
    std::weak_ptr<RequestData> weak_data;
    {
        auto data = make_request_data();
        weak_data = data;
        // data goes out of scope, RequestData destroyed
    }
    
    Request req(weak_data);
    
    // All methods should return empty, not crash
    EXPECT_EQ(req.method(), "");
    EXPECT_EQ(req.path(), "");
    EXPECT_EQ(req.body(), "");
    EXPECT_EQ(req.header("content-type"), "");
    EXPECT_EQ(req.path_param("id"), "");
}

TEST_F(RequestHandleTest, RequestGracefulAfterDataExpires) {
    auto data = make_request_data();
    Request req(data);
    
    // Works before expiration
    EXPECT_EQ(req.method(), "POST");
    
    // Expire the data
    data.reset();
    
    // Graceful after expiration
    EXPECT_EQ(req.method(), "");
    EXPECT_EQ(req.path(), "");
}

TEST_F(RequestHandleTest, MultipleRequestsCopiesShareSameData) {
    auto data = make_request_data();
    Request req1(data);
    Request req2 = req1;
    Request req3 = req2;
    
    // Modify underlying data
    data->body = "modified";
    
    // All copies see the change
    EXPECT_EQ(req1.body(), "modified");
    EXPECT_EQ(req2.body(), "modified");
    EXPECT_EQ(req3.body(), "modified");
}

TEST_F(RequestHandleTest, HeaderReturnsEmptyForMissingKey) {
    auto data = make_request_data();
    Request req(data);
    
    EXPECT_EQ(req.header("x-nonexistent"), "");
}

TEST_F(RequestHandleTest, PathParamReturnsEmptyForMissingKey) {
    auto data = make_request_data();
    Request req(data);
    
    EXPECT_EQ(req.path_param("nonexistent"), "");
}
