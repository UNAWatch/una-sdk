/**
 ******************************************************************************
 * @file    KernelTestDoubles_test.cpp
 * @brief   Tests for the shared host-test doubles' instrumentation, so the
 *          assertions other suites build on it stay trustworthy.
 ******************************************************************************
 */

#include "KernelTestDoubles.hpp"

#include <gtest/gtest.h>

using SDK::TestSupport::InMemoryFileSystem;

// The open-handle instrumentation is keyed by path: an IFile::rename() on an
// OPEN handle must migrate the count to the new path. Without the migration
// the old bucket stays nonzero forever (a phantom leak) and the eventual
// close() drains the new bucket it never incremented.
TEST(InMemoryFileSystem, RenameWhileOpenMigratesOpenHandleCount)
{
    InMemoryFileSystem fs;
    fs.seedFile("dir/a.txt", "payload");

    auto f = fs.file("dir/a.txt");
    ASSERT_TRUE(f->open(/*wMode=*/false));
    EXPECT_EQ(fs.openHandles["dir/a.txt"], 1u);

    ASSERT_TRUE(f->rename("dir/b.txt"));
    EXPECT_EQ(fs.openHandles["dir/a.txt"], 0u) << "old bucket released";
    EXPECT_EQ(fs.openHandles["dir/b.txt"], 1u) << "count follows the handle";

    EXPECT_TRUE(f->close());
    EXPECT_EQ(fs.openHandles["dir/b.txt"], 0u) << "no underflow after close";
}

// A rename of a CLOSED handle must leave the instrumentation untouched.
TEST(InMemoryFileSystem, RenameWhileClosedLeavesHandleCountsAlone)
{
    InMemoryFileSystem fs;
    fs.seedFile("dir/a.txt", "payload");

    auto f = fs.file("dir/a.txt");
    ASSERT_TRUE(f->rename("dir/b.txt"));
    EXPECT_EQ(fs.openHandles["dir/a.txt"], 0u);
    EXPECT_EQ(fs.openHandles["dir/b.txt"], 0u);
}

// The closeGate fault hook fails an open handle's close() and leaves it open
// (FatFs f_close semantics: a failed sync keeps the FIL valid and its
// lock-table entry held); once the gate is lifted, close() drains normally.
TEST(InMemoryFileSystem, CloseGateFailsCloseAndKeepsHandleOpen)
{
    InMemoryFileSystem fs;
    fs.seedFile("dir/a.txt", "payload");

    auto f = fs.file("dir/a.txt");
    ASSERT_TRUE(f->open(/*wMode=*/false));

    fs.closeGate = [](const std::string&) { return false; };
    EXPECT_FALSE(f->close());
    EXPECT_TRUE(f->isOpen()) << "a failed close keeps the handle open";
    EXPECT_EQ(fs.openHandles["dir/a.txt"], 1u) << "lock still held";

    fs.closeGate = nullptr;
    EXPECT_TRUE(f->close());
    EXPECT_EQ(fs.openHandles["dir/a.txt"], 0u);
}
