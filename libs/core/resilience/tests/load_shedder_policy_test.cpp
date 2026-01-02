#include "resilience/policy/LoadShedderPolicy.h"

#include <gtest/gtest.h>

using namespace astra::resilience;

TEST(LoadShedderPolicyTest, CreateWithValidValues) {
  auto policy = LoadShedderPolicy::create(100, "test_shedder");

  EXPECT_EQ(policy.max_concurrent, 100);
  EXPECT_EQ(policy.name, "test_shedder");
}

TEST(LoadShedderPolicyTest, CreateThrowsOnZeroMaxConcurrent) {
  EXPECT_THROW(LoadShedderPolicy::create(0, "invalid"), std::invalid_argument);
}

TEST(LoadShedderPolicyTest, CreateWithMinimumValidValue) {
  auto policy = LoadShedderPolicy::create(1, "minimum");

  EXPECT_EQ(policy.max_concurrent, 1);
}

TEST(LoadShedderPolicyTest, CreateWithLargeValue) {
  auto policy = LoadShedderPolicy::create(1000000, "large");

  EXPECT_EQ(policy.max_concurrent, 1000000);
}
