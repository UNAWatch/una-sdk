/**
 * Simulator builds share one FixedQueue between the GUI comm thread and the
 * SDL tick thread. The watch does not define SIMULATOR, and this file
 * compiles the header the way the simulator does.
 */
#define SIMULATOR 1

#include "SDK/Tools/FixedQueue.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <thread>
#include <vector>

TEST(FixedQueue, PushPopKeepsOrder) {
    SDK::Tools::FixedQueue<int, 4> q;
    EXPECT_TRUE(q.empty());
    EXPECT_TRUE(q.push(1));
    EXPECT_TRUE(q.push(2));
    EXPECT_EQ(q.size(), 2u);
    EXPECT_EQ(q.pop(), 1);
    EXPECT_EQ(q.pop(), 2);
    EXPECT_FALSE(q.pop());
    EXPECT_TRUE(q.empty());
}

TEST(FixedQueue, FullPushFailsAndAssignmentClears) {
    SDK::Tools::FixedQueue<int, 2> q;
    EXPECT_TRUE(q.push(1));
    EXPECT_TRUE(q.push(2));
    EXPECT_TRUE(q.full());
    EXPECT_FALSE(q.push(3));
    q = {};
    EXPECT_TRUE(q.empty());
    EXPECT_FALSE(q.pop());
}

TEST(FixedQueue, TwoThreadsDoNotLoseOrDuplicate) {
    constexpr int n = 4000;
    SDK::Tools::FixedQueue<int, 8> q;
    std::thread producer([&] {
        for (int i = 0; i < n;) {
            if (q.push(i)) {
                ++i;
            }
        }
    });

    std::vector<int> got;
    got.reserve(static_cast<size_t>(n));
    while (static_cast<int>(got.size()) < n) {
        if (auto value = q.pop()) {
            got.push_back(*value);
        }
    }
    producer.join();

    std::sort(got.begin(), got.end());
    ASSERT_EQ(static_cast<int>(got.size()), n);
    for (int i = 0; i < n; ++i) {
        ASSERT_EQ(got[static_cast<size_t>(i)], i);
    }
    EXPECT_TRUE(q.empty());
}
