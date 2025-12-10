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
 * Handler signature tests for HTTP/2 Server
 * Verifies Request& and Response& handler interface
 */

// Test 1: Handler receives references to concrete types
TEST(HandlerSignatureTest, HandlerReceivesReferences) {
    auto server = std::make_unique<http2server::Server>("127.0.0.1", "9100", 1);
    
    bool handler_called = false;
    
    // Handler signature: Request&, Response&
    server->handle("GET", "/test", 
        [&](http2server::Request& req, http2server::Response& res) {
            handler_called = true;
            res.close();
        });
    
    // If compilation succeeds, test passes
    EXPECT_TRUE(true);
}

// Test 2: Multiple handlers can be registered
TEST(HandlerSignatureTest, MultipleHandlers) {
    auto server = std::make_unique<http2server::Server>("127.0.0.1", "9101", 1);
    
    server->handle("GET", "/path1", 
        [](http2server::Request& req, http2server::Response& res) {
            res.close();
        });
    
    server->handle("POST", "/path2",
        [](http2server::Request& req, http2server::Response& res) {
            res.close();
        });
    
    SUCCEED();
}

// Test 3: Handler can access request and response methods
TEST(HandlerSignatureTest, AccessRequestResponseMethods) {
    auto server = std::make_unique<http2server::Server>("127.0.0.1", "9102", 1);
    
    server->handle("GET", "/test",
        [](http2server::Request& req, http2server::Response& res) {
            // Access request methods
            [[maybe_unused]] auto path = req.path();
            [[maybe_unused]] auto method = req.method();
            
            // Access response methods
            res.set_status(200);
            res.write("OK");
            res.close();
        });
    
    SUCCEED();
}

// Test 4: Router integration
TEST(HandlerSignatureTest, RouterIntegration) {
    auto server = std::make_unique<http2server::Server>("127.0.0.1", "9103", 1);
    
    // Access router directly
    server->router().get("/route", [](router::IRequest& req, router::IResponse& res) {
        res.set_status(200);
        res.close();
    });
    
    SUCCEED();
}

// Test 5: Handler signature matches Server::Handler type
TEST(HandlerSignatureTest, HandlerTypeCompatible) {
    http2server::Server::Handler handler = 
        [](http2server::Request& req, http2server::Response& res) {
            res.close();
        };
    
    EXPECT_TRUE(handler != nullptr);
}
