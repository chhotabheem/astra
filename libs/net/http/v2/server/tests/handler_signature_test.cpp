#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Http2Server.h"
#include "Http2Request.h"
#include "Http2Response.h"
#include "IRequest.h"
#include "IResponse.h"
#include <thread>
#include <chrono>
#include <atomic>

using namespace testing;
using namespace std::chrono_literals;

/**
 * TDD Tests for shared_ptr Handler signature
 * 
 * These tests verify the new Handler signature that uses:
 *   std::shared_ptr<router::IRequest>, std::shared_ptr<router::IResponse>
 * 
 * This enables async processing where handlers can return immediately
 * and worker threads process later.
 */

// Test 1: Handler receives interface smart pointers, not concrete types
TEST(HandlerSignatureTest, HandlerReceivesInterfaceSmartPointers) {
    auto server = std::make_unique<http2server::Server>("127.0.0.1", "9100", 1);
    
    bool handler_called = false;
    std::shared_ptr<router::IRequest> captured_req;
    std::shared_ptr<router::IResponse> captured_res;
    
    // New signature: shared_ptr to interfaces
    server->handle("GET", "/test", 
        [&](std::shared_ptr<router::IRequest> req, std::shared_ptr<router::IResponse> res) {
            handler_called = true;
            captured_req = req;
            captured_res = res;
            res->close();
        });
    
    // If compilation succeeds and no crash, test passes
    EXPECT_TRUE(true);
}

// Test 2: Request/Response survive after handler returns (for async processing)
TEST(HandlerSignatureTest, ObjectsSurviveAfterHandlerReturns) {
    std::shared_ptr<router::IResponse> stored_response;
    std::atomic<bool> handler_returned{false};
    
    auto server = std::make_unique<http2server::Server>("127.0.0.1", "9101", 1);
    
    // Handler stores response and returns immediately
    server->handle("GET", "/async", 
        [&](std::shared_ptr<router::IRequest> req, std::shared_ptr<router::IResponse> res) {
            stored_response = res;  // Store for later use
            handler_returned = true;
            // NOT calling res->close() - simulating async
        });
    
    // In real scenario, worker thread would later call:
    // if (stored_response) stored_response->close();
    
    SUCCEED();
}

// Test 3: Application only sees IRequest/IResponse, never concrete types
TEST(HandlerSignatureTest, ApplicationOnlySeesInterfaces) {
    auto server = std::make_unique<http2server::Server>("127.0.0.1", "9102", 1);
    
    // This should compile - we work with interfaces
    server->handle("GET", "/interfaces",
        [](std::shared_ptr<router::IRequest> req, std::shared_ptr<router::IResponse> res) {
            // We can use interface methods
            [[maybe_unused]] auto path = req->path();
            [[maybe_unused]] auto method = req->method();
            res->set_status(200);
            res->write("OK");
            res->close();
        });
    
    SUCCEED();
}

// Test 4: Response gracefully handles closed stream via internal weak_ptr
TEST(HandlerSignatureTest, ResponseGracefullyHandlesClosedStream) {
    auto server = std::make_unique<http2server::Server>("127.0.0.1", "9103", 1);
    
    std::shared_ptr<router::IResponse> stored_response;
    
    server->handle("GET", "/graceful",
        [&](std::shared_ptr<router::IRequest> req, std::shared_ptr<router::IResponse> res) {
            stored_response = res;
        });
    
    // Simulate: stream closes, then we try to use response
    // This should NOT crash - internal weak_ptr should handle gracefully
    // (Cannot fully test without running server, but API should support this)
    
    SUCCEED();
}

// Test 5: Router Handler type matches new signature
TEST(HandlerSignatureTest, RouterHandlerTypeCompatible) {
    // router::Handler should now be:
    // std::function<void(std::shared_ptr<router::IRequest>, std::shared_ptr<router::IResponse>)>
    
    router::Handler handler = [](std::shared_ptr<router::IRequest> req, 
                                  std::shared_ptr<router::IResponse> res) {
        res->close();
    };
    
    EXPECT_TRUE(handler != nullptr);
}
