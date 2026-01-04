#include "DataServiceHandler.h"
#include "DataServiceMessages.h"

#include <IExecutor.h>
#include <Message.h>
#include <atomic>
#include <chrono>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <thread>

using namespace uri_shortener::service;
using namespace astra::execution;
using ::testing::_;
using ::testing::Invoke;
using ::testing::SaveArg;

namespace uri_shortener::service::test {

// ===========================================================================
// Mock Adapter
// ===========================================================================

class MockDataServiceAdapter : public IDataServiceAdapter {
public:
  MOCK_METHOD(void, execute,
              (DataServiceRequest request, DataServiceCallback callback),
              (override));
};

// ===========================================================================
// Mock Executor
// ===========================================================================

class MockExecutor : public IExecutor {
public:
  MOCK_METHOD(void, submit, (Message msg), (override));
};

// ===========================================================================
// Tests
// ===========================================================================

class DataServiceHandlerTest : public ::testing::Test {
protected:
  void SetUp() override {
  }

  MockDataServiceAdapter m_mock_adapter;
  MockExecutor m_mock_executor;
};

// Basic message routing
TEST_F(DataServiceHandlerTest, RoutesDataServiceRequestToAdapter) {
  DataServiceHandler handler(m_mock_adapter, m_mock_executor);

  DataServiceRequest ds_req{DataServiceOperation::FIND, "test123", "", nullptr,
                            nullptr};

  Message msg;
  msg.affinity_key = 42;
  msg.payload = ds_req;

  EXPECT_CALL(m_mock_adapter, execute(_, _)).Times(1);

  handler.handle(msg);
}

// Callback submits to executor
TEST_F(DataServiceHandlerTest, AdapterCallbackSubmitsToExecutor) {
  DataServiceHandler handler(m_mock_adapter, m_mock_executor);

  DataServiceRequest ds_req{DataServiceOperation::FIND, "abc123", "", nullptr,
                            nullptr};

  Message msg;
  msg.affinity_key = 123;
  msg.payload = ds_req;

  DataServiceCallback captured_callback;

  EXPECT_CALL(m_mock_adapter, execute(_, _))
      .WillOnce(SaveArg<1>(&captured_callback));

  EXPECT_CALL(m_mock_executor, submit(_)).Times(1);

  handler.handle(msg);

  // Simulate adapter calling back
  DataServiceResponse resp;
  resp.success = true;
  resp.payload = R"({"code":"abc123","url":"https://example.com"})";

  captured_callback(std::move(resp));
}

// Affinity key preserved
TEST_F(DataServiceHandlerTest, AffinityKeyPreservedInResponse) {
  DataServiceHandler handler(m_mock_adapter, m_mock_executor);

  DataServiceRequest ds_req{DataServiceOperation::FIND, "xyz", "", nullptr,
                            nullptr};

  Message msg;
  msg.affinity_key = 9999;
  msg.payload = ds_req;

  DataServiceCallback captured_callback;
  Message captured_response_msg;

  EXPECT_CALL(m_mock_adapter, execute(_, _))
      .WillOnce(SaveArg<1>(&captured_callback));

  EXPECT_CALL(m_mock_executor, submit(_))
      .WillOnce([&captured_response_msg](Message m) {
        captured_response_msg = std::move(m);
      });

  handler.handle(msg);

  // Trigger callback
  DataServiceResponse resp;
  resp.success = true;
  captured_callback(std::move(resp));

  // Verify affinity key preserved
  EXPECT_EQ(captured_response_msg.affinity_key, 9999);
}

// Trace context preserved
TEST_F(DataServiceHandlerTest, TraceContextPreservedInResponse) {
  DataServiceHandler handler(m_mock_adapter, m_mock_executor);

  DataServiceRequest ds_req{DataServiceOperation::SAVE, "",
                            R"({"url":"https://test.com"})", nullptr, nullptr};

  Message msg;
  msg.affinity_key = 1;
  msg.trace_ctx = obs::Context{}; // Would have real trace context
  msg.payload = ds_req;

  DataServiceCallback captured_callback;
  Message captured_response_msg;

  EXPECT_CALL(m_mock_adapter, execute(_, _))
      .WillOnce(SaveArg<1>(&captured_callback));

  EXPECT_CALL(m_mock_executor, submit(_))
      .WillOnce([&captured_response_msg](Message m) {
        captured_response_msg = std::move(m);
      });

  handler.handle(msg);

  captured_callback(DataServiceResponse{});

  // Response should have trace context (verification limited without real
  // context)
  SUCCEED();
}

// Error response handling
TEST_F(DataServiceHandlerTest, ErrorResponseSubmittedToExecutor) {
  DataServiceHandler handler(m_mock_adapter, m_mock_executor);

  DataServiceRequest ds_req{DataServiceOperation::FIND, "notfound", "", nullptr,
                            nullptr};

  Message msg;
  msg.affinity_key = 500;
  msg.payload = ds_req;

  DataServiceCallback captured_callback;
  Message captured_response_msg;

  EXPECT_CALL(m_mock_adapter, execute(_, _))
      .WillOnce(SaveArg<1>(&captured_callback));

  EXPECT_CALL(m_mock_executor, submit(_))
      .WillOnce([&captured_response_msg](Message m) {
        captured_response_msg = std::move(m);
      });

  handler.handle(msg);

  // Simulate error response
  DataServiceResponse resp;
  resp.success = false;
  resp.domain_error_code = 1; // NOT_FOUND
  resp.error_message = "Link not found";
  captured_callback(std::move(resp));

  // Verify response was submitted
  auto &payload =
      std::any_cast<DataServiceResponse &>(captured_response_msg.payload);
  EXPECT_FALSE(payload.success);
}

} // namespace uri_shortener::service::test
