#include "Url.h"

#include <gtest/gtest.h>

using namespace astra::utils;

TEST(UrlTest, ParseQueryString_Empty) {
  auto result = Url::parse_query_string("");
  EXPECT_TRUE(result.empty());
}

TEST(UrlTest, ParseQueryString_SingleParam) {
  auto result = Url::parse_query_string("foo=bar");
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result["foo"], "bar");
}

TEST(UrlTest, ParseQueryString_MultipleParams) {
  auto result = Url::parse_query_string("a=1&b=2&c=3");
  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result["a"], "1");
  EXPECT_EQ(result["b"], "2");
  EXPECT_EQ(result["c"], "3");
}

TEST(UrlTest, ParseQueryString_UrlEncodedSpace) {
  auto result = Url::parse_query_string("name=hello%20world");
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result["name"], "hello world");
}

TEST(UrlTest, ParseQueryString_UrlEncodedSpecialChars) {
  auto result = Url::parse_query_string("email=test%40example.com");
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result["email"], "test@example.com");
}

TEST(UrlTest, ParseQueryString_EmptyValue) {
  auto result = Url::parse_query_string("flag=");
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result["flag"], "");
}

TEST(UrlTest, ParseQueryString_NoValue) {
  auto result = Url::parse_query_string("flag");
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result["flag"], "");
}

TEST(UrlTest, ParseQueryString_DuplicateKeysLastWins) {
  auto result = Url::parse_query_string("a=1&a=2");
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result["a"], "2");
}
