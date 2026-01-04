#include "ObservableMessageHandler.h"
#include "ObservableRequestHandler.h"
#include "UriMessages.h"

#include <Context.h>
#include <Http2Request.h>
#include <Http2Response.h>
#include <IMessageHandler.h>
#include <Message.h>
#include <Span.h>
#include <atomic>
#include <gtest/gtest.h>

using namespace uri_shortener;
using namespace astra::execution;

/**
 * TDD Tests for Observable Handler Decorators
 */

// Mock inner handler
class MockInnerHandler : public IMessageHandler {
public:
  void handle(Message &msg) override {
    m_handled_count++;
    m_last_affinity_key = msg.affinity_key;
    if (m_should_throw) {
      throw std::runtime_error("Test error");
    }
  }

  std::atomic<int> m_handled_count{0};
  uint64_t m_last_affinity_key{0};
  bool m_should_throw{false};
};

class ObservableHandlerTest : public ::testing::Test {
protected:
  MockInnerHandler inner;
};

TEST_F(ObservableHandlerTest, DelegatesToInnerHandler) {
  ObservableMessageHandler observable(inner);

  auto req = std::make_shared<astra::http2::Http2Request>();
  auto resp = std::make_shared<astra::http2::Http2Response>();
  HttpRequestMsg http_msg{req, resp};

  Message msg{42, obs::Context::create(), UriPayload{std::move(http_msg)}};

  observable.handle(msg);

  EXPECT_EQ(inner.m_handled_count, 1);
  EXPECT_EQ(inner.m_last_affinity_key, 42);
}

TEST_F(ObservableHandlerTest, HandlesMultipleMessages) {
  ObservableMessageHandler observable(inner);

  for (int i = 0; i < 5; ++i) {
    auto req = std::make_shared<astra::http2::Http2Request>();
    auto resp = std::make_shared<astra::http2::Http2Response>();
    Message msg{static_cast<uint64_t>(i), obs::Context::create(),
                UriPayload{HttpRequestMsg{req, resp}}};
    observable.handle(msg);
  }

  EXPECT_EQ(inner.m_handled_count, 5);
}

TEST_F(ObservableHandlerTest, PropagatesExceptionsAfterRecording) {
  inner.m_should_throw = true;
  ObservableMessageHandler observable(inner);

  auto req = std::make_shared<astra::http2::Http2Request>();
  auto resp = std::make_shared<astra::http2::Http2Response>();
  Message msg{1, obs::Context::create(), UriPayload{HttpRequestMsg{req, resp}}};

  EXPECT_THROW(observable.handle(msg), std::runtime_error);

  // Handler was still called
  EXPECT_EQ(inner.m_handled_count, 1);
}

TEST_F(ObservableHandlerTest, HandlesNonUriPayload) {
  ObservableMessageHandler observable(inner);

  // Non-UriPayload message
  Message msg{1, obs::Context::create(), std::string("not a UriPayload")};

  // Should not throw, just skip type attribute
  observable.handle(msg);

  EXPECT_EQ(inner.m_handled_count, 1);
}
