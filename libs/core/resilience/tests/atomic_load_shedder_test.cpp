#include "resilience/impl/AtomicLoadShedder.h"
#include "resilience/policy/LoadShedderPolicy.h"

#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <random>
#include <thread>
#include <vector>

using namespace astra::resilience;

class AtomicLoadShedderTest : public ::testing::Test {
protected:
  LoadShedderPolicy policy = LoadShedderPolicy::create(5, "test");
};

TEST_F(AtomicLoadShedderTest, AcquireSucceedsWhenUnderLimit) {
  AtomicLoadShedder shedder(policy);

  auto guard = shedder.try_acquire();

  ASSERT_TRUE(guard.has_value());
  EXPECT_EQ(shedder.current_count(), 1);
}

TEST_F(AtomicLoadShedderTest, AcquireFailsWhenAtLimit) {
  auto small_policy = LoadShedderPolicy::create(2, "small");
  AtomicLoadShedder shedder(small_policy);

  auto guard1 = shedder.try_acquire();
  auto guard2 = shedder.try_acquire();
  EXPECT_EQ(shedder.current_count(), 2);

  auto guard3 = shedder.try_acquire();
  EXPECT_FALSE(guard3.has_value());
  EXPECT_EQ(shedder.current_count(), 2);
}

TEST_F(AtomicLoadShedderTest, CountDecrementsWhenGuardDestroyed) {
  AtomicLoadShedder shedder(policy);

  {
    auto guard = shedder.try_acquire();
    EXPECT_EQ(shedder.current_count(), 1);
  }

  EXPECT_EQ(shedder.current_count(), 0);
}

TEST_F(AtomicLoadShedderTest, UpdatePolicyChangesMaxConcurrent) {
  AtomicLoadShedder shedder(policy);
  EXPECT_EQ(shedder.max_concurrent(), 5);

  auto new_policy = LoadShedderPolicy::create(10, "updated");
  shedder.update_policy(new_policy);

  EXPECT_EQ(shedder.max_concurrent(), 10);
}

TEST_F(AtomicLoadShedderTest, AcquireAllAndRelease) {
  auto small_policy = LoadShedderPolicy::create(3, "small");
  AtomicLoadShedder shedder(small_policy);

  std::vector<std::optional<LoadShedderGuard>> guards;
  for (int i = 0; i < 3; ++i) {
    guards.push_back(shedder.try_acquire());
    EXPECT_TRUE(guards.back().has_value());
  }
  EXPECT_EQ(shedder.current_count(), 3);

  // Should fail now
  auto extra = shedder.try_acquire();
  EXPECT_FALSE(extra.has_value());

  // Release one
  guards.pop_back();
  EXPECT_EQ(shedder.current_count(), 2);

  // Should succeed again
  auto another = shedder.try_acquire();
  EXPECT_TRUE(another.has_value());
  EXPECT_EQ(shedder.current_count(), 3);
}

TEST_F(AtomicLoadShedderTest, ThreadSafetyConcurrentAcquireRelease) {
  auto large_policy = LoadShedderPolicy::create(100, "concurrent");
  AtomicLoadShedder shedder(large_policy);

  std::atomic<int> successful_acquires{0};
  std::atomic<int> failed_acquires{0};

  auto worker = [&]() {
    for (int i = 0; i < 50; ++i) {
      auto guard = shedder.try_acquire();
      if (guard) {
        successful_acquires++;
        std::this_thread::yield(); // Allow interleaving
      } else {
        failed_acquires++;
      }
    }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < 10; ++i) {
    threads.emplace_back(worker);
  }

  for (auto &t : threads) {
    t.join();
  }

  EXPECT_EQ(shedder.current_count(), 0);    // All released
  EXPECT_GT(successful_acquires.load(), 0); // Some succeeded
}

// ============================================================================
// STRESS TESTS
// ============================================================================

TEST_F(AtomicLoadShedderTest, HeavyStressTest) {
  // 100 threads, 1000 ops each = 100,000 total operations
  auto stress_policy = LoadShedderPolicy::create(50, "stress");
  AtomicLoadShedder shedder(stress_policy);

  constexpr int NUM_THREADS = 100;
  constexpr int OPS_PER_THREAD = 1000;

  std::atomic<size_t> total_acquires{0};
  std::atomic<size_t> total_rejections{0};
  std::atomic<size_t> max_seen_concurrent{0};

  auto worker = [&]() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> hold_time(0, 10);

    for (int i = 0; i < OPS_PER_THREAD; ++i) {
      auto guard = shedder.try_acquire();
      if (guard) {
        total_acquires++;

        // Track max concurrent we've seen
        size_t current = shedder.current_count();
        size_t expected = max_seen_concurrent.load();
        while (current > expected &&
               !max_seen_concurrent.compare_exchange_weak(expected, current)) {
        }

        // Hold for random short time
        if (hold_time(gen) > 8) {
          std::this_thread::yield();
        }
      } else {
        total_rejections++;
      }
    }
  };

  std::vector<std::thread> threads;
  threads.reserve(NUM_THREADS);
  for (int i = 0; i < NUM_THREADS; ++i) {
    threads.emplace_back(worker);
  }

  for (auto &t : threads) {
    t.join();
  }

  // Verify invariants
  EXPECT_EQ(shedder.current_count(), 0) << "All guards must be released";
  EXPECT_EQ(total_acquires + total_rejections, NUM_THREADS * OPS_PER_THREAD)
      << "Every op must result in acquire or rejection";
  EXPECT_LE(max_seen_concurrent, 50) << "Never exceeded max_concurrent";
  EXPECT_GT(total_acquires, 0) << "Some acquires should succeed";
}

TEST_F(AtomicLoadShedderTest, PolicyUpdateDuringHighLoad) {
  // Start with low limit, increase during load
  auto initial_policy = LoadShedderPolicy::create(10, "initial");
  AtomicLoadShedder shedder(initial_policy);

  std::atomic<bool> stop{false};
  std::atomic<size_t> acquires_before_update{0};
  std::atomic<size_t> acquires_after_update{0};
  std::atomic<bool> policy_updated{false};

  auto worker = [&]() {
    while (!stop) {
      auto guard = shedder.try_acquire();
      if (guard) {
        if (policy_updated) {
          acquires_after_update++;
        } else {
          acquires_before_update++;
        }
        std::this_thread::yield();
      }
    }
  };

  // Start workers
  std::vector<std::thread> threads;
  for (int i = 0; i < 20; ++i) {
    threads.emplace_back(worker);
  }

  // Let it run briefly
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Update policy to higher limit
  auto new_policy = LoadShedderPolicy::create(100, "updated");
  shedder.update_policy(new_policy);
  policy_updated = true;

  // Let it run more
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  stop = true;
  for (auto &t : threads) {
    t.join();
  }

  EXPECT_EQ(shedder.current_count(), 0);
  EXPECT_EQ(shedder.max_concurrent(), 100);
  EXPECT_GT(acquires_before_update, 0);
  EXPECT_GT(acquires_after_update, 0);
}

TEST_F(AtomicLoadShedderTest, ReduceMaxBelowCurrentInFlight) {
  // Start with high limit
  auto initial_policy = LoadShedderPolicy::create(10, "high");
  AtomicLoadShedder shedder(initial_policy);

  // Acquire 5 guards
  std::vector<std::optional<LoadShedderGuard>> guards;
  for (int i = 0; i < 5; ++i) {
    guards.push_back(shedder.try_acquire());
    ASSERT_TRUE(guards.back().has_value());
  }
  EXPECT_EQ(shedder.current_count(), 5);

  // Reduce max to 2 (below current 5)
  auto reduced_policy = LoadShedderPolicy::create(2, "reduced");
  shedder.update_policy(reduced_policy);

  // Existing guards should still be valid (don't force-release)
  EXPECT_EQ(shedder.current_count(), 5);
  EXPECT_EQ(shedder.max_concurrent(), 2);

  // New acquires should fail until count drops below new max
  auto new_guard = shedder.try_acquire();
  EXPECT_FALSE(new_guard.has_value());

  // Release 4 guards (count goes to 1, below new max of 2)
  guards.clear();
  guards.resize(1); // Keep only first

  // Wait for guards to actually release
  guards.clear();
  EXPECT_EQ(shedder.current_count(), 0);

  // Now acquire should succeed
  auto after_release = shedder.try_acquire();
  EXPECT_TRUE(after_release.has_value());
}

// NOTE: GuardOutlivesLoadShedder test was removed.
// It intentionally triggered use-after-free (UB) which sanitizers correctly
// detect. In production: Ensure guards never outlive their LoadShedder!

TEST_F(AtomicLoadShedderTest, RapidAcquireReleaseCycles) {
  // Test rapid acquire/release doesn't cause issues
  auto rapid_policy = LoadShedderPolicy::create(1, "rapid");
  AtomicLoadShedder shedder(rapid_policy);

  for (int i = 0; i < 10000; ++i) {
    auto guard = shedder.try_acquire();
    ASSERT_TRUE(guard.has_value()) << "Failed at iteration " << i;
    EXPECT_EQ(shedder.current_count(), 1);
  } // All guards released by end of each iteration

  EXPECT_EQ(shedder.current_count(), 0);
}

TEST_F(AtomicLoadShedderTest, ZeroInFlightAfterMixedOperations) {
  // Complex sequence of operations
  AtomicLoadShedder shedder(policy);

  auto g1 = shedder.try_acquire();
  auto g2 = shedder.try_acquire();

  // Move g1 to g3
  auto g3 = std::move(g1);

  // Release g2
  g2.reset();
  EXPECT_EQ(shedder.current_count(), 1);

  // Acquire more
  auto g4 = shedder.try_acquire();
  auto g5 = shedder.try_acquire();
  EXPECT_EQ(shedder.current_count(), 3);

  // Clear all
  g3.reset();
  g4.reset();
  g5.reset();

  EXPECT_EQ(shedder.current_count(), 0);
}
