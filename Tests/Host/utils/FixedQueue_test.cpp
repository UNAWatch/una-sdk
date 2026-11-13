/**
 * Simulator builds share one FixedQueue between the GUI comm thread and the
 * SDL tick thread. The watch does not define SIMULATOR, and this file
 * compiles the header the way the simulator does.
 */
#define SIMULATOR 1

#include "SDK/Tools/FixedQueue.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <mutex>
#include <thread>
#include <utility>
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

    ASSERT_EQ(static_cast<int>(got.size()), n);
    for (int i = 0; i < n; ++i) {
        ASSERT_EQ(got[static_cast<size_t>(i)], i);
    }
    EXPECT_TRUE(q.empty());
}

namespace {

struct NotAssignable {
    static int copies;
    NotAssignable() = default;
    NotAssignable(const NotAssignable&) { ++copies; }
    NotAssignable& operator=(const NotAssignable&) = delete;
};

int NotAssignable::copies = 0;

struct MoveOnly {
    static int moves;
    MoveOnly() = default;
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&&) noexcept { ++moves; }
    MoveOnly& operator=(MoveOnly&&) = delete;
};

int MoveOnly::moves = 0;

}  // namespace

TEST(FixedQueue, CopyConstructsElementsThatCannotBeAssigned) {
    NotAssignable::copies = 0;
    SDK::Tools::FixedQueue<NotAssignable, 3> q;
    SDK::Tools::FixedQueue<NotAssignable, 3> copy(q);
    EXPECT_EQ(NotAssignable::copies, 3);
    EXPECT_TRUE(copy.empty());
    EXPECT_TRUE(q.empty());
}

TEST(FixedQueue, MoveConstructsElementsThatCannotBeAssigned) {
    MoveOnly::moves = 0;
    SDK::Tools::FixedQueue<MoveOnly, 2> q;
    SDK::Tools::FixedQueue<MoveOnly, 2> moved(std::move(q));
    EXPECT_EQ(MoveOnly::moves, 2);
    EXPECT_TRUE(moved.empty());
    EXPECT_TRUE(q.empty());
}

TEST(FixedQueue, PushEvictingOldestDropsTheHeadOnly) {
    SDK::Tools::FixedQueue<int, 2> q;
    EXPECT_FALSE(q.pushEvictingOldest(1).has_value());
    EXPECT_FALSE(q.pushEvictingOldest(2).has_value());
    auto dropped = q.pushEvictingOldest(3);
    ASSERT_TRUE(dropped.has_value());
    EXPECT_EQ(*dropped, 1);
    EXPECT_TRUE(q.full());
    EXPECT_EQ(q.pop(), 2);
    EXPECT_EQ(q.pop(), 3);
    EXPECT_TRUE(q.empty());
}

TEST(FixedQueue, PushEvictingOldestKeepsEachValueOnce) {
    constexpr int n = 3000;
    SDK::Tools::FixedQueue<int, 8> q;
    std::vector<int> dropped;
    std::mutex droppedMutex;
    std::atomic<bool> done{false};

    std::thread producer([&] {
        for (int i = 0; i < n; ++i) {
            if (auto value = q.pushEvictingOldest(i)) {
                std::lock_guard<std::mutex> lock(droppedMutex);
                dropped.push_back(*value);
            }
        }
        done.store(true);
    });

    std::vector<int> got;
    got.reserve(static_cast<size_t>(n));
    while (!done.load() || !q.empty()) {
        if (auto value = q.pop()) {
            got.push_back(*value);
        }
    }
    producer.join();
    while (auto value = q.pop()) {
        got.push_back(*value);
    }

    std::vector<int> all;
    all.reserve(static_cast<size_t>(n));
    all.insert(all.end(), got.begin(), got.end());
    all.insert(all.end(), dropped.begin(), dropped.end());
    std::sort(all.begin(), all.end());
    ASSERT_EQ(static_cast<int>(all.size()), n);
    for (int i = 0; i < n; ++i) {
        ASSERT_EQ(all[static_cast<size_t>(i)], i);
    }
}
