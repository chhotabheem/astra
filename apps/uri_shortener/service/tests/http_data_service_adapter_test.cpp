#include "Http2Client.h"
#include "HttpDataServiceAdapter.h"
#include "StaticServiceResolver.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <mutex>
#include <thread>

using namespace uri_shortener::service;
using namespace astra::http2;
using namespace astra::service_discovery;
using ::testing::_;
using ::testing::Invoke;

namespace uri_shortener::service::test {

class HttpDataServiceAdapterTest : public ::testing::Test {
protected:
  void SetUp() override {
    m_config.set_request_timeout_ms(100);

    // Setup resolver with test backend
    m_resolver.register_service("dataservice", "127.0.0.1", 29999);
  }

  ::http2::ClientConfig m_config;
  StaticServiceResolver m_resolver;
};

// ===========================================================================
// Operation Translation Tests
// ===========================================================================

TEST_F(HttpDataServiceAdapterTest, SaveTranslatesToPost) {
  Http2Client client(m_config);
  HttpDataServiceAdapter adapter(client, m_resolver, "dataservice");

  DataServiceRequest req{DataServiceOperation::SAVE,
                         "", // entity_id not needed for SAVE
                         R"({"code":"abc123","url":"https://example.com"})",
                         nullptr, nullptr};

  std::atomic<bool> callback_called{false};

  adapter.execute(req, [&callback_called](DataServiceResponse resp) {
    callback_called = true;
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  EXPECT_TRUE(callback_called);
}

TEST_F(HttpDataServiceAdapterTest, FindTranslatesToGet) {
  Http2Client client(m_config);
  HttpDataServiceAdapter adapter(client, m_resolver, "dataservice");

  DataServiceRequest req{DataServiceOperation::FIND, "abc123", "", nullptr,
                         nullptr};

  std::atomic<bool> callback_called{false};

  adapter.execute(req, [&callback_called](DataServiceResponse resp) {
    callback_called = true;
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  EXPECT_TRUE(callback_called);
}

TEST_F(HttpDataServiceAdapterTest, DeleteTranslatesToDelete) {
  Http2Client client(m_config);
  HttpDataServiceAdapter adapter(client, m_resolver, "dataservice");

  DataServiceRequest req{DataServiceOperation::DELETE, "xyz789", "", nullptr,
                         nullptr};

  std::atomic<bool> callback_called{false};

  adapter.execute(req, [&callback_called](DataServiceResponse resp) {
    callback_called = true;
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  EXPECT_TRUE(callback_called);
}

// ===========================================================================
// Error Handling Tests
// ===========================================================================

TEST_F(HttpDataServiceAdapterTest, ConnectionFailureReturnsInfraError) {
  Http2Client client(m_config);
  HttpDataServiceAdapter adapter(client, m_resolver, "dataservice");

  DataServiceRequest req{DataServiceOperation::FIND, "abc123", "", nullptr,
                         nullptr};

  std::mutex mtx;
  std::condition_variable cv;
  bool callback_called = false;
  DataServiceResponse captured_response;

  adapter.execute(req, [&mtx, &cv, &callback_called,
                        &captured_response](DataServiceResponse resp) {
    std::lock_guard<std::mutex> lock(mtx);
    captured_response = std::move(resp);
    callback_called = true;
    cv.notify_one();
  });

  {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait_for(lock, std::chrono::milliseconds(500), [&callback_called] {
      return callback_called;
    });
  }

  EXPECT_TRUE(callback_called);
  EXPECT_FALSE(captured_response.success);
  EXPECT_TRUE(captured_response.infra_error.has_value());
}

TEST_F(HttpDataServiceAdapterTest, UnknownServiceThrows) {
  Http2Client client(m_config);
  HttpDataServiceAdapter adapter(client, m_resolver, "nonexistent-service");

  DataServiceRequest req{DataServiceOperation::FIND, "abc123", "", nullptr,
                         nullptr};

  EXPECT_THROW(
      { adapter.execute(req, [](DataServiceResponse) {}); },
      std::runtime_error);
}

// ===========================================================================
// Response Handle Passthrough Tests
// ===========================================================================

TEST_F(HttpDataServiceAdapterTest, ResponseHandlePreservedInCallback) {
  Http2Client client(m_config);
  HttpDataServiceAdapter adapter(client, m_resolver, "dataservice");

  DataServiceRequest req{DataServiceOperation::FIND, "abc123", "", nullptr,
                         nullptr};

  std::atomic<bool> callback_called{false};

  adapter.execute(req, [&callback_called](DataServiceResponse resp) {
    callback_called = true;
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  EXPECT_TRUE(callback_called);
}

// ===========================================================================
// Path Building Tests
// ===========================================================================

TEST_F(HttpDataServiceAdapterTest, CustomBasePathUsed) {
  Http2Client client(m_config);
  HttpDataServiceAdapter::Config adapter_config{"/custom/api/links"};
  HttpDataServiceAdapter adapter(client, m_resolver, "dataservice",
                                 adapter_config);

  DataServiceRequest req{DataServiceOperation::FIND, "test123", "", nullptr,
                         nullptr};

  std::atomic<bool> callback_called{false};

  adapter.execute(req, [&callback_called](DataServiceResponse resp) {
    callback_called = true;
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  EXPECT_TRUE(callback_called);
}

// ===========================================================================
// Concurrent Request Tests
// ===========================================================================

TEST_F(HttpDataServiceAdapterTest, ConcurrentRequestsHandledCorrectly) {
  Http2Client client(m_config);
  HttpDataServiceAdapter adapter(client, m_resolver, "dataservice");

  std::atomic<int> callback_count{0};
  const int num_requests = 10;

  for (int i = 0; i < num_requests; i++) {
    DataServiceRequest req{DataServiceOperation::FIND,
                           "code" + std::to_string(i), "", nullptr, nullptr};

    adapter.execute(req, [&callback_count](DataServiceResponse resp) {
      callback_count++;
    });
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  EXPECT_EQ(callback_count, num_requests);
}

// ===========================================================================
// Service Resolver Integration Tests
// ===========================================================================

TEST_F(HttpDataServiceAdapterTest, MultipleServicesCanBeRegistered) {
  m_resolver.register_service("service-a", "127.0.0.1", 29001);
  m_resolver.register_service("service-b", "127.0.0.1", 29002);

  Http2Client client(m_config);
  HttpDataServiceAdapter adapter_a(client, m_resolver, "service-a");
  HttpDataServiceAdapter adapter_b(client, m_resolver, "service-b");

  std::atomic<int> callback_count{0};

  DataServiceRequest req{DataServiceOperation::FIND, "test", "", nullptr,
                         nullptr};

  adapter_a.execute(req, [&](DataServiceResponse) {
    callback_count++;
  });
  adapter_b.execute(req, [&](DataServiceResponse) {
    callback_count++;
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  EXPECT_EQ(callback_count, 2);
}

} // namespace uri_shortener::service::test
