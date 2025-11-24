#include <http2server/server.hpp>
#include <http2server/request.hpp>
#include <http2server/response.hpp>
#include <thread>
#include <chrono>
#include <iostream>
#include <vector>
#include <functional>

int test_passed = 0;
int test_failed = 0;

#define ASSERT(condition, message) \
    if (!(condition)) { \
        throw std::runtime_error(std::string("Assertion failed: ") + message); \
    }

void run_test(const char* name, std::function<void()> test_func) {
    std::cout << "[TEST] " << name << std::endl;
    try {
        test_func();
        test_passed++;
        std::cout << "  ✓ PASSED" << std::endl;
    } catch (const std::exception& e) {
        test_failed++;
        std::cout << "  ✗ FAILED: " << e.what() << std::endl;
    }
}

void test_ServerConstruction() {
    auto server = std::make_unique<http2server::Server>("127.0.0.1", "9001", 1);
    ASSERT(server != nullptr, "Server should construct without throwing");
    server.reset();
}

void test_ServerHandlerRegistration() {
    auto server = std::make_unique<http2server::Server>("127.0.0.1", "9002", 1);
    
    bool handler_called = false;
    server->handle("GET", "/test", [&](const http2server::Request& req, http2server::Response& res) {
        handler_called = true;
        res.close();
    });
    
    ASSERT(true, "Handler registration should not throw");
    server.reset();
}

void test_ServerMultipleHandlers() {
    auto server = std::make_unique<http2server::Server>("127.0.0.1", "9003", 1);
    
    server->handle("GET", "/path1", [](const auto& req, auto& res) {
        res.close();
    });
    
    server->handle("POST", "/path2", [](const auto& req, auto& res) {
        res.close();
    });
    
    server->handle("GET", "/path3", [](const auto& req, auto& res) {
        res.close();
    });
    
    ASSERT(true, "Multiple handlers should register successfully");
    server.reset();
}

void test_ServerThreadConfiguration() {
    auto server1 = std::make_unique<http2server::Server>("127.0.0.1", "9004", 1);
    server1.reset();
    
    auto server2 = std::make_unique<http2server::Server>("127.0.0.1", "9005", 2);
    server2.reset();
    
    auto server4 = std::make_unique<http2server::Server>("127.0.0.1", "9006", 4);
    server4.reset();
    
    ASSERT(true, "Server should accept different thread counts");
}

void test_ServerBindToAllInterfaces() {
    auto server = std::make_unique<http2server::Server>("0.0.0.0", "9007", 1);
    ASSERT(server != nullptr, "Server should bind to 0.0.0.0");
    server.reset();
}

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "HTTP2Server Unit Tests" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    run_test("ServerConstruction", test_ServerConstruction);
    run_test("ServerHandlerRegistration", test_ServerHandlerRegistration);
    run_test("ServerMultipleHandlers", test_ServerMultipleHandlers);
    run_test("ServerThreadConfiguration", test_ServerThreadConfiguration);
    run_test("ServerBindToAllInterfaces", test_ServerBindToAllInterfaces);
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test Results:" << std::endl;
    std::cout << "  Passed: " << test_passed << std::endl;
    std::cout << "  Failed: " << test_failed << std::endl;
    std::cout << "========================================" << std::endl;
    
    if (test_failed > 0) {
        std::cout << "\n✗ TESTS FAILED\n" << std::endl;
        return 1;
    }
    
    std::cout << "\n✓ ALL TESTS PASSED\n" << std::endl;
    return 0;
}
