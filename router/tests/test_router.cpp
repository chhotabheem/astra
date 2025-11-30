#include "Router.h"
#include <iostream>
#include <cassert>
#include <string>

using namespace router;

void test_exact_match() {
    std::cout << "Running test_exact_match..." << std::endl;
    Router router;
    bool called = false;
    
    router.get("/users", [&](IRequest&, IResponse&) {
        called = true;
    });
    
    auto result = router.match("GET", "/users");
    assert(result.handler != nullptr);
    assert(result.params.empty());
    
    // Simulate call
    // result.handler(req, res); // We don't have mock req/res here, just checking match
    
    std::cout << "PASSED" << std::endl;
}

void test_param_match() {
    std::cout << "Running test_param_match..." << std::endl;
    Router router;
    
    router.get("/users/:id", [&](IRequest&, IResponse&) {});
    
    auto result = router.match("GET", "/users/123");
    assert(result.handler != nullptr);
    assert(result.params.size() == 1);
    assert(result.params["id"] == "123");
    
    std::cout << "PASSED" << std::endl;
}

void test_nested_params() {
    std::cout << "Running test_nested_params..." << std::endl;
    Router router;
    
    router.get("/users/:userId/posts/:postId", [&](IRequest&, IResponse&) {});
    
    auto result = router.match("GET", "/users/123/posts/456");
    assert(result.handler != nullptr);
    assert(result.params.size() == 2);
    assert(result.params["userId"] == "123");
    assert(result.params["postId"] == "456");
    
    std::cout << "PASSED" << std::endl;
}

void test_collision_priority() {
    std::cout << "Running test_collision_priority..." << std::endl;
    Router router;
    
    bool static_called = false;
    bool dynamic_called = false;
    
    router.get("/users/profile", [&](IRequest&, IResponse&) { static_called = true; });
    router.get("/users/:id", [&](IRequest&, IResponse&) { dynamic_called = true; });
    
    // Test Static Priority
    auto result_static = router.match("GET", "/users/profile");
    assert(result_static.handler != nullptr);
    assert(result_static.params.empty()); // Should NOT match :id
    
    // Test Dynamic Fallback
    auto result_dynamic = router.match("GET", "/users/123");
    assert(result_dynamic.handler != nullptr);
    assert(result_dynamic.params["id"] == "123");
    
    std::cout << "PASSED" << std::endl;
}

void test_no_match() {
    std::cout << "Running test_no_match..." << std::endl;
    Router router;
    
    router.get("/users", [&](IRequest&, IResponse&) {});
    
    auto result = router.match("GET", "/unknown");
    assert(result.handler == nullptr);
    
    auto result_method = router.match("POST", "/users");
    assert(result_method.handler == nullptr);
    
    std::cout << "PASSED" << std::endl;
}

int main() {
    test_exact_match();
    test_param_match();
    test_nested_params();
    test_collision_priority();
    test_no_match();
    
    std::cout << "All Router tests passed!" << std::endl;
    return 0;
}
