/// @file test_short_link.cpp
/// @brief TDD tests for ShortLink entity (Aggregate Root)

#include "ExpirationPolicy.h"
#include "OriginalUrl.h"
#include "ShortCode.h"
#include "ShortLink.h"

#include <chrono>
#include <gtest/gtest.h>

namespace uri_shortener::domain::test {

using namespace std::chrono_literals;

// =============================================================================
// Factory Method Tests
// =============================================================================

TEST(ShortLinkTest, Create_WithValidInputs_Succeeds) {
  auto code = ShortCode::create("abc123").value();
  auto url = OriginalUrl::create("https://example.com").value();
  auto policy = ExpirationPolicy::never();

  auto result = ShortLink::create(code, url, policy);

  ASSERT_TRUE(result.is_ok());
  auto link = result.value();
  EXPECT_EQ(link.code(), code);
  EXPECT_EQ(link.original(), url);
}

TEST(ShortLinkTest, Create_DefaultsToNeverExpire) {
  auto code = ShortCode::create("abc123").value();
  auto url = OriginalUrl::create("https://example.com").value();

  auto result = ShortLink::create(code, url);

  ASSERT_TRUE(result.is_ok());
  EXPECT_FALSE(result.value().expiration().expires());
}

// =============================================================================
// Expiration Tests
// =============================================================================

TEST(ShortLinkTest, IsExpired_WhenNeverExpires_ReturnsFalse) {
  auto code = ShortCode::create("abc123").value();
  auto url = OriginalUrl::create("https://example.com").value();
  auto policy = ExpirationPolicy::never();

  auto link = ShortLink::create(code, url, policy).value();

  EXPECT_FALSE(link.is_expired());
}

TEST(ShortLinkTest, IsExpired_BeforeExpiry_ReturnsFalse) {
  auto code = ShortCode::create("abc123").value();
  auto url = OriginalUrl::create("https://example.com").value();
  auto policy = ExpirationPolicy::after(std::chrono::hours(24));

  auto link = ShortLink::create(code, url, policy).value();

  EXPECT_FALSE(link.is_expired());
}

TEST(ShortLinkTest, IsActive_WhenNotExpired_ReturnsTrue) {
  auto code = ShortCode::create("abc123").value();
  auto url = OriginalUrl::create("https://example.com").value();
  auto policy = ExpirationPolicy::never();

  auto link = ShortLink::create(code, url, policy).value();

  EXPECT_TRUE(link.is_active());
}

// =============================================================================
// Created At Tests
// =============================================================================

TEST(ShortLinkTest, CreatedAt_IsSetOnCreation) {
  auto before = std::chrono::system_clock::now();

  auto code = ShortCode::create("abc123").value();
  auto url = OriginalUrl::create("https://example.com").value();
  auto link = ShortLink::create(code, url).value();

  auto after = std::chrono::system_clock::now();

  EXPECT_GE(link.created_at(), before);
  EXPECT_LE(link.created_at(), after);
}

// =============================================================================
// Equality Tests
// =============================================================================

TEST(ShortLinkTest, SameCode_AreEqual) {
  auto code = ShortCode::create("abc123").value();
  auto url1 = OriginalUrl::create("https://example1.com").value();
  auto url2 = OriginalUrl::create("https://example2.com").value();

  auto link1 = ShortLink::create(code, url1).value();
  auto link2 = ShortLink::create(code, url2).value();

  // Links with same code are considered equal (code is the identity)
  EXPECT_EQ(link1, link2);
}

TEST(ShortLinkTest, DifferentCode_AreNotEqual) {
  auto code1 = ShortCode::create("abc123").value();
  auto code2 = ShortCode::create("xyz789").value();
  auto url = OriginalUrl::create("https://example.com").value();

  auto link1 = ShortLink::create(code1, url).value();
  auto link2 = ShortLink::create(code2, url).value();

  EXPECT_NE(link1, link2);
}

} // namespace uri_shortener::domain::test
