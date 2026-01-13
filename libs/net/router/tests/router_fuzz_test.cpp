#include "Router.h"
#include "fuzztest/fuzztest.h"

#include "gtest/gtest.h"
#include <string>

using namespace astra::router;

// =============================================================================
// Fuzz Targets
// =============================================================================

// Core fuzz target: match() should never crash
void MatchNeverCrashes(const std::string &method, const std::string &path) {
  Router router;

  // Add some routes
  router.add(HttpMethod::GET, "/users", [](auto, auto) {});
  router.add(HttpMethod::GET, "/users/:id", [](auto, auto) {});
  router.add(HttpMethod::POST, "/users", [](auto, auto) {});
  router.add(HttpMethod::GET, "/posts/:postId/comments/:commentId",
             [](auto, auto) {});

  // Match with fuzzed input - should never crash
  auto result = router.match(method, path);
  (void)result; // Ignore result, just verify no crash
}
FUZZ_TEST(RouterFuzzTest, MatchNeverCrashes);

// Fuzz adding routes with random paths
void AddRouteNeverCrashes(const std::string &path) {
  Router router;

  try {
    router.add(HttpMethod::GET, path, [](auto, auto) {});
  } catch (const std::exception &) {
    // Allowed
  }
}
FUZZ_TEST(RouterFuzzTest, AddRouteNeverCrashes);

// Fuzz with path parameters
void MatchWithParams(const std::string &userId, const std::string &action) {
  Router router;
  router.add(HttpMethod::GET, "/users/:userId/:action", [](auto, auto) {});

  std::string path = "/users/" + userId + "/" + action;
  auto result = router.match("GET", path);

  // If matched, params should be extracted
  if (result && result->handler) {
    // Access params - should never crash
    auto it1 = result->params.find("userId");
    auto it2 = result->params.find("action");
    (void)it1;
    (void)it2;
  }
}
FUZZ_TEST(RouterFuzzTest, MatchWithParams);

// Fuzz all HTTP methods
void MatchAllMethods(const std::string &path) {
  Router router;
  router.add(HttpMethod::GET, "/test", [](auto, auto) {});
  router.add(HttpMethod::POST, "/test", [](auto, auto) {});
  router.add(HttpMethod::PUT, "/test", [](auto, auto) {});
  router.add(HttpMethod::DELETE, "/test", [](auto, auto) {});

  // Try all methods with fuzzed path
  (void)router.match("GET", path);
  (void)router.match("POST", path);
  (void)router.match("PUT", path);
  (void)router.match("DELETE", path);
  (void)router.match("PATCH", path);
  (void)router.match("OPTIONS", path);
  (void)router.match("HEAD", path);
  (void)router.match(path, path); // Method could be anything
}
FUZZ_TEST(RouterFuzzTest, MatchAllMethods);
