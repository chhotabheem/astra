#include "Http2Request.h"

#include <gtest/gtest.h>
#include <memory>

using namespace astra::http2;

class Http2RequestTest : public ::testing::Test {
protected:
  Http2Request make_request() {
    return Http2Request(
        "POST", "/api/shorten",
        {{"content-type", "application/json"}, {"accept", "application/json"}},
        R"({"url":"https://example.com"})", {{"page", "1"}});
  }
};

TEST_F(Http2RequestTest, ConstructorSetsAllFields) {
  auto req = make_request();

  EXPECT_EQ(req.method(), "POST");
  EXPECT_EQ(req.path(), "/api/shorten");
  EXPECT_EQ(req.body(), R"({"url":"https://example.com"})");
  EXPECT_EQ(req.header("content-type"), "application/json");
  EXPECT_EQ(req.header("accept"), "application/json");
  EXPECT_EQ(req.query_param("page"), "1");
}

TEST_F(Http2RequestTest, DefaultConstructorCreatesEmptyRequest) {
  Http2Request req;

  EXPECT_EQ(req.method(), "");
  EXPECT_EQ(req.path(), "");
  EXPECT_EQ(req.body(), "");
}

TEST_F(Http2RequestTest, RequestIsCopyable) {
  auto req1 = make_request();

  Http2Request req2 = req1;
  Http2Request req3(req1);

  EXPECT_EQ(req2.method(), "POST");
  EXPECT_EQ(req3.method(), "POST");
}

TEST_F(Http2RequestTest, RequestIsMovable) {
  auto req1 = make_request();

  Http2Request req2 = std::move(req1);

  EXPECT_EQ(req2.method(), "POST");
  EXPECT_EQ(req2.path(), "/api/shorten");
}

TEST_F(Http2RequestTest, HeaderReturnsEmptyForMissingKey) {
  auto req = make_request();

  EXPECT_EQ(req.header("x-nonexistent"), "");
}

TEST_F(Http2RequestTest, QueryParamReturnsEmptyForMissingKey) {
  auto req = make_request();

  EXPECT_EQ(req.query_param("nonexistent"), "");
}

TEST_F(Http2RequestTest, SetPathParamsWorks) {
  auto req = make_request();

  req.set_path_params({{"id", "abc123"}, {"code", "xyz"}});

  EXPECT_EQ(req.path_param("id"), "abc123");
  EXPECT_EQ(req.path_param("code"), "xyz");
}

TEST_F(Http2RequestTest, PathParamReturnsEmptyForMissingKey) {
  auto req = make_request();

  EXPECT_EQ(req.path_param("nonexistent"), "");
}

TEST_F(Http2RequestTest, EmptyBodyIsValid) {
  Http2Request req("GET", "/health");

  EXPECT_EQ(req.method(), "GET");
  EXPECT_EQ(req.path(), "/health");
  EXPECT_EQ(req.body(), "");
}

TEST_F(Http2RequestTest, SharedPtrUsage) {
  auto req = std::make_shared<Http2Request>("GET", "/test");

  EXPECT_EQ(req->method(), "GET");
  EXPECT_EQ(req->path(), "/test");
}
