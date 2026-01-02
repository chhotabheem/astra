#include "resilience/LoadShedderGuard.h"

#include <gtest/gtest.h>

using namespace astra::resilience;

class LoadShedderGuardTest : public ::testing::Test {
protected:
  int release_count = 0;

  LoadShedderGuard::ReleaseFn make_release_fn() {
    return [this]() {
      ++release_count;
    };
  }
};

TEST_F(LoadShedderGuardTest, ReleasesOnDestruction) {
  {
    auto guard = LoadShedderGuard::create(make_release_fn());
    EXPECT_EQ(release_count, 0);
  }
  EXPECT_EQ(release_count, 1);
}

TEST_F(LoadShedderGuardTest, MoveDoesNotDoubleRelease) {
  {
    auto guard1 = LoadShedderGuard::create(make_release_fn());
    auto guard2 = std::move(guard1);
    EXPECT_EQ(release_count, 0);
  }
  EXPECT_EQ(release_count, 1); // Only one release
}

TEST_F(LoadShedderGuardTest, MoveAssignmentReleasesOldGuard) {
  {
    auto guard1 = LoadShedderGuard::create(make_release_fn());
    auto guard2 = LoadShedderGuard::create(make_release_fn());
    EXPECT_EQ(release_count, 0);

    guard1 = std::move(guard2); // guard1's original should release
    EXPECT_EQ(release_count, 1);
  }
  EXPECT_EQ(release_count, 2); // Both released
}

TEST_F(LoadShedderGuardTest, MovedFromGuardDoesNotReleaseOnDestruction) {
  {
    auto guard1 = LoadShedderGuard::create(make_release_fn());
    {
      auto guard2 = std::move(guard1);
    } // guard2 destroyed, releases
    EXPECT_EQ(release_count, 1);
  } // guard1 destroyed (moved-from), should not release again
  EXPECT_EQ(release_count, 1);
}

TEST_F(LoadShedderGuardTest, SelfMoveAssignmentIsSafe) {
  {
    auto guard = LoadShedderGuard::create(make_release_fn());
    guard = std::move(guard); // Self-assignment
    EXPECT_EQ(release_count, 0);
  }
  EXPECT_EQ(release_count, 1);
}
