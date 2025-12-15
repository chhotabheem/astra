#include "fuzztest/fuzztest.h"
#include "gtest/gtest.h"
#include "Router.h"
#include <string>

using namespace astra::router;

// =============================================================================
// Fuzz Targets
// =============================================================================

// Core fuzz target: match() should never crash
void MatchNeverCrashes(const std::string& method, const std::string& path) {
    Router router;
    
    // Add some routes
    router.get("/users", [](auto, auto) {});
    router.get("/users/:id", [](auto, auto) {});
    router.post("/users", [](auto, auto) {});
    router.get("/posts/:postId/comments/:commentId", [](auto, auto) {});
    
    // Match with fuzzed input - should never crash
    auto result = router.match(method, path);
    (void)result;  // Ignore result, just verify no crash
}
FUZZ_TEST(RouterFuzzTest, MatchNeverCrashes);

// Fuzz adding routes with random paths
void AddRouteNeverCrashes(const std::string& path) {
    Router router;
    
    try {
        router.get(path, [](auto, auto) {});
    } catch (const std::exception&) {
        // Allowed
    }
}
FUZZ_TEST(RouterFuzzTest, AddRouteNeverCrashes);

// Fuzz with path parameters
void MatchWithParams(const std::string& userId, const std::string& action) {
    Router router;
    router.get("/users/:userId/:action", [](auto, auto) {});
    
    std::string path = "/users/" + userId + "/" + action;
    auto result = router.match("GET", path);
    
    // If matched, params should be extracted
    if (result.handler) {
        // Access params - should never crash
        auto it1 = result.params.find("userId");
        auto it2 = result.params.find("action");
        (void)it1;
        (void)it2;
    }
}
FUZZ_TEST(RouterFuzzTest, MatchWithParams);

// Fuzz all HTTP methods
void MatchAllMethods(const std::string& path) {
    Router router;
    router.get("/test", [](auto, auto) {});
    router.post("/test", [](auto, auto) {});
    router.put("/test", [](auto, auto) {});
    router.del("/test", [](auto, auto) {});
    
    // Try all methods with fuzzed path
    router.match("GET", path);
    router.match("POST", path);
    router.match("PUT", path);
    router.match("DELETE", path);
    router.match("PATCH", path);
    router.match("OPTIONS", path);
    router.match("HEAD", path);
    router.match(path, path);  // Method could be anything
}
FUZZ_TEST(RouterFuzzTest, MatchAllMethods);
