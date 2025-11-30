#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Http1Client.h"

using namespace testing;

class Http1ClientTest : public Test {
protected:
    http1::Client client_;
};

TEST_F(Http1ClientTest, ConnectionRefused) {
    // Connect to a port that is likely closed
    auto res = client_.get("127.0.0.1", "1", "/"); 
    EXPECT_EQ(res.status_code, 500);
}

TEST_F(Http1ClientTest, InvalidHost) {
    auto res = client_.get("invalid.host.local", "80", "/");
    EXPECT_EQ(res.status_code, 500);
}
