#include "SDK/AppSystem/AlignedAlloc.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdlib>
#include <functional>

using SDK::AppSystem::alignedAlloc;
using SDK::AppSystem::alignedRawPointer;

namespace {

struct OffsetHeap {
    std::uintptr_t offset;
    void* last = nullptr;
    alignas(64) unsigned char arena[256] = {};

    void* operator()(std::size_t)
    {
        return last = arena + offset;
    }
};

}

TEST(AlignedAlloc, ResultIsAlignedAndFreesWhatTheAllocatorReturned)
{
    for (std::size_t align : {16u, 32u, 64u}) {
        for (std::uintptr_t offset : {0u, 8u}) {
            OffsetHeap heap{offset};
            void* p = alignedAlloc(24, align, std::ref(heap));
            ASSERT_NE(p, nullptr);
            EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p) % align, 0u);
            EXPECT_EQ(alignedRawPointer(p), heap.last);
        }
    }
}

TEST(AlignedAlloc, BlockFitsInsideWhatWasAllocated)
{
    std::size_t requested = 0;
    void* raw = nullptr;
    void* p = alignedAlloc(100, 32, [&](std::size_t n) {
        requested = n;
        return raw = std::malloc(n);
    });
    ASSERT_NE(p, nullptr);
    const auto begin = reinterpret_cast<std::uintptr_t>(raw);
    const auto start = reinterpret_cast<std::uintptr_t>(p);
    EXPECT_GE(start - sizeof(void*), begin);
    EXPECT_LE(start + 100, begin + requested);
    std::free(alignedRawPointer(p));
}

TEST(AlignedAlloc, AllocatorFailureReturnsNull)
{
    EXPECT_EQ(alignedAlloc(16, 16, [](std::size_t) -> void* { return nullptr; }), nullptr);
}

TEST(AlignedAlloc, SizeOverflowReturnsNullWithoutAllocating)
{
    bool called = false;
    void* p = alignedAlloc(SIZE_MAX - 4, 16, [&](std::size_t) -> void* {
        called = true;
        return nullptr;
    });
    EXPECT_EQ(p, nullptr);
    EXPECT_FALSE(called);
}
