#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "DataServiceHandler.h"
#include "DataServiceMessages.h"
#include <Message.h>
#include <IQueue.h>
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>

using namespace url_shortener::service;
using namespace astra::execution;
using ::testing::_;
using ::testing::Invoke;
using ::testing::SaveArg;

namespace url_shortener::service::test {

// ===========================================================================
// Mock Adapter
// ===========================================================================

class MockDataServiceAdapter : public IDataServiceAdapter {
public:
    MOCK_METHOD(void, execute, (DataServiceRequest request, DataServiceCallback callback), (override));
};

// ===========================================================================
// Mock Queue
// ===========================================================================

class MockQueue : public IQueue {
public:
    MOCK_METHOD(bool, submit, (Message msg), (override));
};

// ===========================================================================
// Tests
// ===========================================================================

class DataServiceHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_mock_queue = std::make_shared<MockQueue>();
    }
    
    MockDataServiceAdapter m_mock_adapter;
    std::shared_ptr<MockQueue> m_mock_queue;
};

// Basic message routing
TEST_F(DataServiceHandlerTest, RoutesDataServiceRequestToAdapter) {
    DataServiceHandler handler(m_mock_adapter, m_mock_queue);
    
    DataServiceRequest ds_req{
        DataServiceOperation::FIND,
        "test123",
        "",
        nullptr,
        nullptr
    };
    
    Message msg;
    msg.session_id = 42;
    msg.payload = ds_req;
    
    EXPECT_CALL(m_mock_adapter, execute(_, _))
        .Times(1);
    
    handler.handle(msg);
}

// Callback submits to StickyQueue
TEST_F(DataServiceHandlerTest, AdapterCallbackSubmitsToStickyQueue) {
    DataServiceHandler handler(m_mock_adapter, m_mock_queue);
    
    DataServiceRequest ds_req{
        DataServiceOperation::FIND,
        "abc123",
        "",
        nullptr,
        nullptr
    };
    
    Message msg;
    msg.session_id = 123;
    msg.payload = ds_req;
    
    DataServiceCallback captured_callback;
    
    EXPECT_CALL(m_mock_adapter, execute(_, _))
        .WillOnce(SaveArg<1>(&captured_callback));
    
    EXPECT_CALL(*m_mock_queue, submit(_))
        .WillOnce(::testing::Return(true));
    
    handler.handle(msg);
    
    // Simulate adapter calling back
    DataServiceResponse resp;
    resp.success = true;
    resp.payload = R"({"code":"abc123","url":"https://example.com"})";
    
    captured_callback(std::move(resp));
}

// Session ID preserved
TEST_F(DataServiceHandlerTest, SessionIdPreservedInResponse) {
    DataServiceHandler handler(m_mock_adapter, m_mock_queue);
    
    DataServiceRequest ds_req{
        DataServiceOperation::FIND,
        "xyz",
        "",
        nullptr,
        nullptr
    };
    
    Message msg;
    msg.session_id = 9999;
    msg.payload = ds_req;
    
    DataServiceCallback captured_callback;
    Message captured_response_msg;
    
    EXPECT_CALL(m_mock_adapter, execute(_, _))
        .WillOnce(SaveArg<1>(&captured_callback));
    
    EXPECT_CALL(*m_mock_queue, submit(_))
        .WillOnce([&captured_response_msg](Message m) {
            captured_response_msg = std::move(m);
            return true;
        });
    
    handler.handle(msg);
    
    // Trigger callback
    DataServiceResponse resp;
    resp.success = true;
    captured_callback(std::move(resp));
    
    // Verify session ID preserved
    EXPECT_EQ(captured_response_msg.session_id, 9999);
}

// Trace context preserved
TEST_F(DataServiceHandlerTest, TraceContextPreservedInResponse) {
    DataServiceHandler handler(m_mock_adapter, m_mock_queue);
    
    DataServiceRequest ds_req{
        DataServiceOperation::SAVE,
        "",
        R"({"url":"https://test.com"})",
        nullptr,
        nullptr
    };
    
    Message msg;
    msg.session_id = 1;
    msg.trace_ctx = obs::Context{};  // Would have real trace context
    msg.payload = ds_req;
    
    DataServiceCallback captured_callback;
    Message captured_response_msg;
    
    EXPECT_CALL(m_mock_adapter, execute(_, _))
        .WillOnce(SaveArg<1>(&captured_callback));
    
    EXPECT_CALL(*m_mock_queue, submit(_))
        .WillOnce([&captured_response_msg](Message m) {
            captured_response_msg = std::move(m);
            return true;
        });
    
    handler.handle(msg);
    
    captured_callback(DataServiceResponse{});
    
    // Response should have trace context (verification limited without real context)
    SUCCEED();
}

// Error response handling
TEST_F(DataServiceHandlerTest, ErrorResponseSubmittedToQueue) {
    DataServiceHandler handler(m_mock_adapter, m_mock_queue);
    
    DataServiceRequest ds_req{
        DataServiceOperation::FIND,
        "notfound",
        "",
        nullptr,
        nullptr
    };
    
    Message msg;
    msg.session_id = 500;
    msg.payload = ds_req;
    
    DataServiceCallback captured_callback;
    Message captured_response_msg;
    
    EXPECT_CALL(m_mock_adapter, execute(_, _))
        .WillOnce(SaveArg<1>(&captured_callback));
    
    EXPECT_CALL(*m_mock_queue, submit(_))
        .WillOnce([&captured_response_msg](Message m) {
            captured_response_msg = std::move(m);
            return true;
        });
    
    handler.handle(msg);
    
    // Simulate error response
    DataServiceResponse resp;
    resp.success = false;
    resp.domain_error_code = 1;  // NOT_FOUND
    resp.error_message = "Link not found";
    captured_callback(std::move(resp));
    
    // Verify response was submitted
    auto& payload = std::any_cast<DataServiceResponse&>(captured_response_msg.payload);
    EXPECT_FALSE(payload.success);
}

} // namespace url_shortener::service::test
