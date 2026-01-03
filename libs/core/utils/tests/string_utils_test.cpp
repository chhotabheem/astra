#include "StringUtils.h"

#include <gtest/gtest.h>

using namespace astra::utils;

TEST(StringUtilsTest, SplitByChar) {
  auto result = split("a/b/c", '/');
  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], "a");
  EXPECT_EQ(result[1], "b");
  EXPECT_EQ(result[2], "c");
}

TEST(StringUtilsTest, SplitSkipsLeadingDelimiter) {
  auto result = split("/a/b", '/');
  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], "a");
  EXPECT_EQ(result[1], "b");
}

TEST(StringUtilsTest, SplitSkipsTrailingDelimiter) {
  auto result = split("a/b/", '/');
  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], "a");
  EXPECT_EQ(result[1], "b");
}

TEST(StringUtilsTest, SplitSkipsConsecutiveDelimiters) {
  auto result = split("a//b", '/');
  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], "a");
  EXPECT_EQ(result[1], "b");
}

TEST(StringUtilsTest, SplitEmptyString) {
  auto result = split("", '/');
  EXPECT_TRUE(result.empty());
}

TEST(StringUtilsTest, SplitNoDelimiter) {
  auto result = split("hello", '/');
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], "hello");
}

TEST(StringUtilsTest, SplitPath) {
  auto result = split("/users/123/posts", '/');
  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], "users");
  EXPECT_EQ(result[1], "123");
  EXPECT_EQ(result[2], "posts");
}

TEST(StringUtilsTest, SplitRootPath) {
  auto result = split("/", '/');
  EXPECT_TRUE(result.empty());
}
