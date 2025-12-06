/// @file test_short_code.cpp
/// @brief TDD tests for ShortCode value object

#include <gtest/gtest.h>
#include "domain/value_objects/ShortCode.h"

namespace url_shortener::domain::test {

// =============================================================================
// Construction Tests
// =============================================================================

TEST(ShortCodeTest, CreateWithValidCode_Succeeds) {
    auto result = ShortCode::create("abc123");
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().value(), "abc123");
}

TEST(ShortCodeTest, CreateWithSixCharacters_Succeeds) {
    auto result = ShortCode::create("abcdef");
    ASSERT_TRUE(result.is_ok());
}

TEST(ShortCodeTest, CreateWithEightCharacters_Succeeds) {
    auto result = ShortCode::create("abcd1234");
    ASSERT_TRUE(result.is_ok());
}

// =============================================================================
// Validation Tests - Length
// =============================================================================

TEST(ShortCodeTest, CreateWithFiveCharacters_Fails) {
    auto result = ShortCode::create("abcde");
    ASSERT_TRUE(result.is_err());
}

TEST(ShortCodeTest, CreateWithNineCharacters_Fails) {
    auto result = ShortCode::create("abcdefghi");
    ASSERT_TRUE(result.is_err());
}

TEST(ShortCodeTest, CreateWithEmptyString_Fails) {
    auto result = ShortCode::create("");
    ASSERT_TRUE(result.is_err());
}

// =============================================================================
// Validation Tests - Character Set
// =============================================================================

TEST(ShortCodeTest, CreateWithSpecialCharacters_Fails) {
    auto result = ShortCode::create("abc@#$");
    ASSERT_TRUE(result.is_err());
}

TEST(ShortCodeTest, CreateWithSpaces_Fails) {
    auto result = ShortCode::create("abc 12");
    ASSERT_TRUE(result.is_err());
}

TEST(ShortCodeTest, CreateWithUppercase_Succeeds) {
    auto result = ShortCode::create("AbCdEf");
    ASSERT_TRUE(result.is_ok());
}

TEST(ShortCodeTest, CreateWithNumbers_Succeeds) {
    auto result = ShortCode::create("123456");
    ASSERT_TRUE(result.is_ok());
}

// =============================================================================
// Equality Tests
// =============================================================================

TEST(ShortCodeTest, EqualCodes_AreEqual) {
    auto code1 = ShortCode::create("abc123").value();
    auto code2 = ShortCode::create("abc123").value();
    EXPECT_EQ(code1, code2);
}

TEST(ShortCodeTest, DifferentCodes_AreNotEqual) {
    auto code1 = ShortCode::create("abc123").value();
    auto code2 = ShortCode::create("xyz789").value();
    EXPECT_NE(code1, code2);
}

// =============================================================================
// Trusted Construction
// =============================================================================

TEST(ShortCodeTest, FromTrusted_DoesNotValidate) {
    // For internal use when loading from persistence
    auto code = ShortCode::from_trusted("abc123");
    EXPECT_EQ(code.value(), "abc123");
}

} // namespace url_shortener::domain::test
