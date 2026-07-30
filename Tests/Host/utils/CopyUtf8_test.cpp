/**
 * Tests for SDK::detail::copyUtf8 -- the UTF-8-safe bounded copy behind
 * HomeWidget's text field. Boundary and truncation behaviour is the only
 * nontrivial logic in the widget wrapper, so it is pinned down here.
 *
 * Bytes are spelled out explicitly (not source string literals) so the cases
 * do not depend on this file's encoding. The two-byte sequence D0 94 is the
 * Cyrillic letter "De"; 94 alone is a bare UTF-8 continuation byte.
 */
#include <gtest/gtest.h>

#include <cstring>

#include "SDK/HomeWidget/HomeWidget.hpp"

namespace {

// Fill with a sentinel so "wrote nothing" and over-writes are both visible.
constexpr char kFill = '#';

// strlen of the result, treating dst as a C string.
std::size_t copyInto(char* dst, std::size_t cap, const char* src)
{
    std::memset(dst, kFill, 16);
    SDK::detail::copyUtf8(dst, cap, src);
    return std::strlen(dst);
}

TEST(CopyUtf8, ZeroCapWritesNothing)
{
    char dst[16];
    std::memset(dst, kFill, sizeof(dst));
    SDK::detail::copyUtf8(dst, 0, "abc");
    EXPECT_EQ(dst[0], kFill);  // buffer untouched
}

TEST(CopyUtf8, CapOneYieldsEmpty)
{
    char dst[16];
    EXPECT_EQ(copyInto(dst, 1, "abc"), 0u);
    EXPECT_EQ(dst[0], '\0');
}

TEST(CopyUtf8, NullSrcYieldsEmpty)
{
    char dst[16];
    EXPECT_EQ(copyInto(dst, 8, nullptr), 0u);
    EXPECT_EQ(dst[0], '\0');
}

TEST(CopyUtf8, FitsWhole)
{
    char dst[16];
    EXPECT_EQ(copyInto(dst, 8, "abc"), 3u);
    EXPECT_STREQ(dst, "abc");
}

TEST(CopyUtf8, ExactFitAscii)
{
    char dst[16];
    // cap 4 holds "abc" + NUL exactly.
    EXPECT_EQ(copyInto(dst, 4, "abc"), 3u);
    EXPECT_STREQ(dst, "abc");
}

TEST(CopyUtf8, AsciiTruncation)
{
    char dst[16];
    // cap 4 -> max 3 payload bytes; 'd' is not a continuation byte, no back-off.
    EXPECT_EQ(copyInto(dst, 4, "abcdef"), 3u);
    EXPECT_STREQ(dst, "abc");
}

TEST(CopyUtf8, TruncationOnCharBoundary)
{
    // "A" + De + De = 41 D0 94 D0 94 ; cap 4 -> max 3 lands after the first De.
    const char src[] = {'\x41', '\xD0', '\x94', '\xD0', '\x94', '\0'};
    const char want[] = {'\x41', '\xD0', '\x94', '\0'};
    char dst[16];
    EXPECT_EQ(copyInto(dst, 4, src), 3u);
    EXPECT_EQ(std::memcmp(dst, want, sizeof(want)), 0);
}

TEST(CopyUtf8, TruncationMidSequenceDropsPartialChar)
{
    // "A" + De = 41 D0 94 ; cap 3 -> max 2 stops mid-De, which must be dropped.
    const char src[] = {'\x41', '\xD0', '\x94', '\0'};
    char dst[16];
    EXPECT_EQ(copyInto(dst, 3, src), 1u);
    EXPECT_STREQ(dst, "A");
}

TEST(CopyUtf8, WholeMultibyteFits)
{
    // "A" + De fits in cap 4 (3 payload bytes + NUL).
    const char src[] = {'\x41', '\xD0', '\x94', '\0'};
    char dst[16];
    EXPECT_EQ(copyInto(dst, 4, src), 3u);
    EXPECT_EQ(std::memcmp(dst, src, 4), 0);
}

TEST(CopyUtf8, AllContinuationBytesDegradeToEmpty)
{
    // Pathological input: only continuation bytes. The n > 0 guard must stop the
    // back-off at 0 (no underflow) and the result is empty, still terminated.
    const char src[] = {'\x94', '\x94', '\x94', '\0'};
    char dst[16];
    EXPECT_EQ(copyInto(dst, 2, src), 0u);
    EXPECT_EQ(dst[0], '\0');
}

TEST(CopyUtf8, MalformedButFittingInputCopiedAsIs)
{
    // Contract: src is assumed valid UTF-8; the copy does not validate it. A
    // malformed byte that fits (here a lone continuation byte) is passed through
    // unchanged -- back-off only fires when truncation would split a sequence.
    const char src[] = {'\x94', '\0'};
    char dst[16];
    EXPECT_EQ(copyInto(dst, 8, src), 1u);
    EXPECT_EQ(std::memcmp(dst, src, 2), 0);
}

}  // namespace
