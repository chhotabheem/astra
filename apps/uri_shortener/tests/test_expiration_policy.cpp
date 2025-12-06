/// @file test_expiration_policy.cpp
/// @brief TDD tests for ExpirationPolicy value object

#include <gtest/gtest.h>
#include "domain/value_objects/ExpirationPolicy.h"
#include <chrono>

namespace url_shortener::domain::test {

using namespace std::chrono_literals;

// =============================================================================
// Factory Method Tests
// =============================================================================

TEST(ExpirationPolicyTest, Never_CreatesNonExpiringPolicy) {
    auto policy = ExpirationPolicy::never();
    EXPECT_FALSE(policy.expires());
}

TEST(ExpirationPolicyTest, AfterDuration_CreatesExpiringPolicy) {
    auto policy = ExpirationPolicy::after(std::chrono::hours(24));
    EXPECT_TRUE(policy.expires());
}

TEST(ExpirationPolicyTest, AtTime_CreatesExpiringPolicy) {
    auto future = std::chrono::system_clock::now() + std::chrono::hours(24);
    auto policy = ExpirationPolicy::at(future);
    EXPECT_TRUE(policy.expires());
}

// =============================================================================
// Expiration Check Tests
// =============================================================================

TEST(ExpirationPolicyTest, Never_NeverExpires) {
    auto policy = ExpirationPolicy::never();
    auto now = std::chrono::system_clock::now();
    auto far_future = now + std::chrono::hours(24 * 365 * 100);  // 100 years
    
    EXPECT_FALSE(policy.has_expired_at(now));
    EXPECT_FALSE(policy.has_expired_at(far_future));
}

TEST(ExpirationPolicyTest, AfterDuration_ExpiresAfterDuration) {
    auto policy = ExpirationPolicy::after(std::chrono::hours(1));
    auto created = policy.created_at();
    
    // Not expired immediately after creation
    EXPECT_FALSE(policy.has_expired_at(created));
    
    // Not expired 30 minutes later
    EXPECT_FALSE(policy.has_expired_at(created + std::chrono::minutes(30)));
    
    // Expired after 1 hour + 1 second
    EXPECT_TRUE(policy.has_expired_at(created + std::chrono::hours(1) + std::chrono::seconds(1)));
}

TEST(ExpirationPolicyTest, AtTime_ExpiresAtSpecifiedTime) {
    auto now = std::chrono::system_clock::now();
    auto expiry_time = now + std::chrono::hours(2);
    auto policy = ExpirationPolicy::at(expiry_time);
    
    // Not expired before expiry time
    EXPECT_FALSE(policy.has_expired_at(now));
    EXPECT_FALSE(policy.has_expired_at(expiry_time - std::chrono::seconds(1)));
    
    // Expired at or after expiry time
    EXPECT_TRUE(policy.has_expired_at(expiry_time + std::chrono::seconds(1)));
}

// =============================================================================
// Expiry Time Accessor Tests
// =============================================================================

TEST(ExpirationPolicyTest, Never_HasNoExpiryTime) {
    auto policy = ExpirationPolicy::never();
    EXPECT_FALSE(policy.expires_at().has_value());
}

TEST(ExpirationPolicyTest, AfterDuration_HasExpiryTime) {
    auto policy = ExpirationPolicy::after(std::chrono::hours(1));
    EXPECT_TRUE(policy.expires_at().has_value());
}

TEST(ExpirationPolicyTest, AtTime_ReturnsCorrectExpiryTime) {
    auto now = std::chrono::system_clock::now();
    auto expiry_time = now + std::chrono::hours(2);
    auto policy = ExpirationPolicy::at(expiry_time);
    
    ASSERT_TRUE(policy.expires_at().has_value());
    // Allow 1 second tolerance for test execution time
    auto diff = std::chrono::abs(policy.expires_at().value() - expiry_time);
    EXPECT_LT(diff, std::chrono::seconds(1));
}

// =============================================================================
// Equality Tests
// =============================================================================

TEST(ExpirationPolicyTest, TwoNeverPolicies_AreEqual) {
    auto policy1 = ExpirationPolicy::never();
    auto policy2 = ExpirationPolicy::never();
    EXPECT_EQ(policy1, policy2);
}

TEST(ExpirationPolicyTest, NeverAndAfter_AreNotEqual) {
    auto policy1 = ExpirationPolicy::never();
    auto policy2 = ExpirationPolicy::after(std::chrono::hours(1));
    EXPECT_NE(policy1, policy2);
}

} // namespace url_shortener::domain::test
