#include "Http2Client.h"
#include "Http2ClientError.h"
#include "Http2ClientResponse.h"
#include "fuzztest/fuzztest.h"

#include "gtest/gtest.h"
#include <atomic>
#include <chrono>
#include <map>
#include <string>
#include <thread>

using namespace astra::http2;

// =============================================================================
// Fuzz Targets
// =============================================================================

// Core fuzz target: submit() should never crash on any input
void SubmitNeverCrashes(const std::string &host, uint16_t port,
                        const std::string &method, const std::string &path,
                        const std::string &body) {
  // Skip empty host or invalid ports that would cause issues
  if (host.empty() || port == 0) {
    return;
  }

  ::http2::ClientConfig config;
  config.set_request_timeout_ms(50);

  try {
    Http2Client client(config);
    std::atomic<bool> done{false};

    client.submit(host, port, method, path, body, {}, [&done](auto) {
      done = true;
    });

    // Brief wait for callback
    for (int i = 0; i < 10 && !done; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  } catch (const std::exception &) {
    // Expected for some inputs - just don't crash
  }
}
FUZZ_TEST(Http2ClientFuzzTest, SubmitNeverCrashes);

// Fuzz with headers
void SubmitWithHeadersNeverCrashes(const std::string &headerKey,
                                   const std::string &headerValue) {
  // Skip problematic header keys
  if (headerKey.empty()) {
    return;
  }

  ::http2::ClientConfig config;
  config.set_request_timeout_ms(50);

  try {
    Http2Client client(config);
    std::atomic<bool> done{false};

    std::map<std::string, std::string> headers;
    headers[headerKey] = headerValue;

    client.submit("127.0.0.1", 19999, "GET", "/test", "", headers,
                  [&done](auto) {
                    done = true;
                  });

    for (int i = 0; i < 10 && !done; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  } catch (const std::exception &) {
    // Allowed
  }
}
FUZZ_TEST(Http2ClientFuzzTest, SubmitWithHeadersNeverCrashes);

// Fuzz response construction
void ResponseConstructionNeverCrashes(int status_code, const std::string &body,
                                      const std::string &headerKey,
                                      const std::string &headerValue) {
  try {
    std::map<std::string, std::string> headers;
    if (!headerKey.empty()) {
      headers[headerKey] = headerValue;
    }

    Http2ClientResponse response(status_code, body, headers);

    // Access methods - should never crash
    (void)response.status_code();
    (void)response.body();
    (void)response.headers();
    (void)response.header(headerKey);
    (void)response.header("nonexistent");
  } catch (const std::exception &) {
    // Allowed
  }
}
FUZZ_TEST(Http2ClientFuzzTest, ResponseConstructionNeverCrashes);

// Fuzz header lookup
void HeaderLookupNeverCrashes(const std::string &lookupKey) {
  std::map<std::string, std::string> headers;
  headers["content-type"] = "application/json";
  headers["x-request-id"] = "12345";

  Http2ClientResponse response(200, "", headers);

  // Should never crash regardless of lookup key
  auto result = response.header(lookupKey);
  (void)result;
}
FUZZ_TEST(Http2ClientFuzzTest, HeaderLookupNeverCrashes);

// Fuzz HTTP methods
void AllMethodsNeverCrash(const std::string &method) {
  ::http2::ClientConfig config;
  config.set_request_timeout_ms(50);

  try {
    Http2Client client(config);
    std::atomic<bool> done{false};

    client.submit("127.0.0.1", 19999, method, "/test", "", {}, [&done](auto) {
      done = true;
    });

    for (int i = 0; i < 10 && !done; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  } catch (const std::exception &) {
    // Allowed
  }
}
FUZZ_TEST(Http2ClientFuzzTest, AllMethodsNeverCrash);

// Fuzz paths with special characters
void PathWithSpecialCharsNeverCrashes(const std::string &path) {
  ::http2::ClientConfig config;
  config.set_request_timeout_ms(50);

  try {
    Http2Client client(config);
    std::atomic<bool> done{false};

    client.submit("127.0.0.1", 19999, "GET", path, "", {}, [&done](auto) {
      done = true;
    });

    for (int i = 0; i < 10 && !done; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  } catch (const std::exception &) {
    // Allowed
  }
}
FUZZ_TEST(Http2ClientFuzzTest, PathWithSpecialCharsNeverCrashes);
