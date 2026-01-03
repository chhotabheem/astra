#include "Http1Client.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class Http1ClientTest : public Test {
protected:
  astra::http1::Client m_client;
};

TEST_F(Http1ClientTest, ConnectionRefused) {
  // Connect to a port that is likely closed
  auto res = m_client.get("127.0.0.1", "1", "/");
  EXPECT_EQ(res.status_code, 500);
}

TEST_F(Http1ClientTest, InvalidHost) {
  auto res = m_client.get("invalid.host.local", "80", "/");
  EXPECT_EQ(res.status_code, 500);
}
