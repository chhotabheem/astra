#include "Http2Request.h"
#include "Http2Response.h"
#include "UriMessages.h"

#include <gtest/gtest.h>
#include <variant>

using namespace uri_shortener;

/**
 * TDD Tests for URI Shortener Messages and Handlers
 */

class UriMessagesTest : public ::testing::Test {};

TEST_F(UriMessagesTest, UriPayloadVariantHoldsHttpRequestMsg) {
  auto req = std::make_shared<astra::http2::Http2Request>();
  auto resp = std::make_shared<astra::http2::Http2Response>();

  UriPayload payload = HttpRequestMsg{req, resp};

  EXPECT_TRUE(std::holds_alternative<HttpRequestMsg>(payload));
}

TEST_F(UriMessagesTest, UriPayloadVariantHoldsDbQueryMsg) {
  auto resp = std::make_shared<astra::http2::Http2Response>();

  UriPayload payload = DbQueryMsg{"shorten", "http://test.com", resp};

  EXPECT_TRUE(std::holds_alternative<DbQueryMsg>(payload));

  auto &query = std::get<DbQueryMsg>(payload);
  EXPECT_EQ(query.operation, "shorten");
  EXPECT_EQ(query.data, "http://test.com");
}

TEST_F(UriMessagesTest, UriPayloadVariantHoldsDbResponseMsg) {
  auto resp = std::make_shared<astra::http2::Http2Response>();

  UriPayload payload = DbResponseMsg{"abc123", true, "", resp};

  EXPECT_TRUE(std::holds_alternative<DbResponseMsg>(payload));

  auto &response = std::get<DbResponseMsg>(payload);
  EXPECT_EQ(response.result, "abc123");
  EXPECT_TRUE(response.success);
}

TEST_F(UriMessagesTest, OverloadedVisitorDispatchesCorrectly) {
  auto resp = std::make_shared<astra::http2::Http2Response>();
  auto req = std::make_shared<astra::http2::Http2Request>();

  std::vector<UriPayload> payloads = {HttpRequestMsg{req, resp},
                                      DbQueryMsg{"resolve", "abc", resp},
                                      DbResponseMsg{"result", true, "", resp}};

  std::vector<int> dispatched;

  auto visitor = overloaded{[&](HttpRequestMsg &) {
                              dispatched.push_back(0);
                            },
                            [&](DbQueryMsg &) {
                              dispatched.push_back(1);
                            },
                            [&](DbResponseMsg &) {
                              dispatched.push_back(2);
                            },
                            [&](service::DataServiceResponse &) {
                              dispatched.push_back(3);
                            }};

  for (auto &payload : payloads) {
    std::visit(visitor, payload);
  }

  ASSERT_EQ(dispatched.size(), 3);
  EXPECT_EQ(dispatched[0], 0);
  EXPECT_EQ(dispatched[1], 1);
  EXPECT_EQ(dispatched[2], 2);
}

TEST_F(UriMessagesTest, DbQueryMsgHoldsData) {
  auto resp = std::make_shared<astra::http2::Http2Response>();

  DbQueryMsg query{"delete", "xyz789", resp};

  EXPECT_EQ(query.operation, "delete");
  EXPECT_EQ(query.data, "xyz789");
}

TEST_F(UriMessagesTest, DbResponseMsgHoldsError) {
  auto resp = std::make_shared<astra::http2::Http2Response>();

  DbResponseMsg response{"", false, "Not found", resp};

  EXPECT_FALSE(response.success);
  EXPECT_EQ(response.error, "Not found");
}
