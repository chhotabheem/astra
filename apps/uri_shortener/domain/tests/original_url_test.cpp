/// @file test_original_url.cpp
/// @brief TDD tests for OriginalUrl value object

#include "OriginalUrl.h"

#include <gtest/gtest.h>

namespace uri_shortener::domain::test {

// =============================================================================
// Construction Tests - Valid URLs
// =============================================================================

TEST(OriginalUrlTest, CreateWithHttpsUrl_Succeeds) {
  auto result = OriginalUrl::create("https://example.com");
  ASSERT_TRUE(result.is_ok());
  EXPECT_EQ(result.value().value(), "https://example.com");
}

TEST(OriginalUrlTest, CreateWithHttpUrl_Succeeds) {
  auto result = OriginalUrl::create("http://example.com");
  ASSERT_TRUE(result.is_ok());
}

TEST(OriginalUrlTest, CreateWithPath_Succeeds) {
  auto result = OriginalUrl::create("https://example.com/path/to/resource");
  ASSERT_TRUE(result.is_ok());
}

TEST(OriginalUrlTest, CreateWithQueryString_Succeeds) {
  auto result =
      OriginalUrl::create("https://example.com/search?q=hello&page=1");
  ASSERT_TRUE(result.is_ok());
}

TEST(OriginalUrlTest, CreateWithPort_Succeeds) {
  auto result = OriginalUrl::create("https://example.com:8080/path");
  ASSERT_TRUE(result.is_ok());
}

TEST(OriginalUrlTest, CreateWithFragment_Succeeds) {
  auto result = OriginalUrl::create("https://example.com/page#section");
  ASSERT_TRUE(result.is_ok());
}

// =============================================================================
// Validation Tests - Invalid URLs
// =============================================================================

TEST(OriginalUrlTest, CreateWithEmptyString_Fails) {
  auto result = OriginalUrl::create("");
  ASSERT_TRUE(result.is_err());
}

TEST(OriginalUrlTest, CreateWithNoScheme_Fails) {
  auto result = OriginalUrl::create("example.com");
  ASSERT_TRUE(result.is_err());
}

TEST(OriginalUrlTest, CreateWithFtpScheme_Fails) {
  // Only http/https supported
  auto result = OriginalUrl::create("ftp://example.com");
  ASSERT_TRUE(result.is_err());
}

TEST(OriginalUrlTest, CreateWithJustScheme_Fails) {
  auto result = OriginalUrl::create("https://");
  ASSERT_TRUE(result.is_err());
}

TEST(OriginalUrlTest, CreateWithSpaces_Fails) {
  auto result = OriginalUrl::create("https://example .com");
  ASSERT_TRUE(result.is_err());
}

TEST(OriginalUrlTest, CreateWithInvalidCharacters_Fails) {
  auto result = OriginalUrl::create("https://exam<ple>.com");
  ASSERT_TRUE(result.is_err());
}

// =============================================================================
// Equality Tests
// =============================================================================

TEST(OriginalUrlTest, EqualUrls_AreEqual) {
  auto url1 = OriginalUrl::create("https://example.com").value();
  auto url2 = OriginalUrl::create("https://example.com").value();
  EXPECT_EQ(url1, url2);
}

TEST(OriginalUrlTest, DifferentUrls_AreNotEqual) {
  auto url1 = OriginalUrl::create("https://example.com").value();
  auto url2 = OriginalUrl::create("https://other.com").value();
  EXPECT_NE(url1, url2);
}

// =============================================================================
// Trusted Construction
// =============================================================================

TEST(OriginalUrlTest, FromTrusted_DoesNotValidate) {
  auto url = OriginalUrl::from_trusted("https://example.com");
  EXPECT_EQ(url.value(), "https://example.com");
}

} // namespace uri_shortener::domain::test
