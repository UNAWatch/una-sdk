/**
 ******************************************************************************
 * @file    KernelTestDoubles_test.cpp
 * @brief   Tests for the shared host-test doubles' instrumentation, so the
 *          assertions other suites build on it stay trustworthy.
 ******************************************************************************
 */

#include "KernelTestDoubles.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

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

// ---------------------------------------------------------------------------
// InMemoryDirectory
//
// The fake's directory enumeration is what makes SDK code that scans a
// directory testable at all (VariantConfig's sandbox-root scan being the
// first such caller). These pin its contract, since suites that build on it
// can only be as trustworthy as it is.
// ---------------------------------------------------------------------------

namespace {

/// Collect a whole directory listing as "name" / "name/" (trailing slash
/// marking a subdirectory), in enumeration order.
std::vector<std::string> listDir(InMemoryFileSystem& fs, const char* path)
{
    std::vector<std::string> out;
    auto dir = fs.dir(path);
    if (!dir || !dir->open()) {
        return out;
    }
    SDK::Interface::IFileSystem::ObjectInfo item{};
    while (dir->readNext(item)) {
        out.push_back(std::string(item.name) + (item.isDir ? "/" : ""));
    }
    dir->close();
    return out;
}

} // namespace

TEST(InMemoryDirectory, ListsDirectChildrenOnly)
{
    InMemoryFileSystem fs;
    fs.seedFile("Activity/morning.fit", "aaa");
    fs.seedFile("Activity/evening.fit", "bb");
    fs.seedFile("Settings/app.json", "x");

    EXPECT_EQ(listDir(fs, "Activity"),
              (std::vector<std::string>{ "evening.fit", "morning.fit" }));
}

TEST(InMemoryDirectory, ReportsSubdirectoriesWithIsDirSet)
{
    InMemoryFileSystem fs;
    fs.seedFile("root/file.txt", "x");
    fs.seedFile("root/nested/deep.txt", "y");

    // "nested" is implied by its child's path -- no mkdir needed -- and must
    // come back flagged as a directory, not as a file.
    EXPECT_EQ(listDir(fs, "root"), (std::vector<std::string>{ "file.txt", "nested/" }));
}

TEST(InMemoryDirectory, DoesNotFlattenNestedFilesIntoTheParent)
{
    InMemoryFileSystem fs;
    fs.seedFile("a/b/c/deep.txt", "x");

    EXPECT_EQ(listDir(fs, "a"), (std::vector<std::string>{ "b/" }));
    EXPECT_EQ(listDir(fs, "a/b"), (std::vector<std::string>{ "c/" }));
    EXPECT_EQ(listDir(fs, "a/b/c"), (std::vector<std::string>{ "deep.txt" }));
}

TEST(InMemoryDirectory, RootCanBeSpelledSlashOrEmpty)
{
    InMemoryFileSystem fs;
    fs.seedFile("a.txt", "x");
    fs.seedFile("sub/inner.txt", "z");

    const std::vector<std::string> expected{ "a.txt", "sub/" };
    EXPECT_EQ(listDir(fs, "/"), expected);
    EXPECT_EQ(listDir(fs, ""), expected) << R"("" and "/" name the same place)";
}

// A documented divergence, pinned so it is a known quantity: leading and
// trailing slashes are not significant, so a file seeded as "/x" is the same
// object as one seeded as "x". This is deliberate -- it is what lets a test
// seed "/App.uapp" and have production code that scans "/" find it -- but a
// real filesystem would not do this.
TEST(InMemoryDirectory, AbsoluteAndRelativeSpellingsShareTheRoot)
{
    InMemoryFileSystem fs;
    fs.seedFile("/leading-slash.txt", "x");
    fs.seedFile("bare.txt", "y");

    EXPECT_EQ(listDir(fs, "/"),
              (std::vector<std::string>{ "bare.txt", "leading-slash.txt" }));
}

// The spellings are interchangeable at every depth, not just at the root, and
// for lookup as well as for enumeration. Both halves matter: a scanner that
// lists a directory and then opens "/" + name is the pattern this fake exists
// to support, and it only works if the two agree.
TEST(InMemoryDirectory, SpellingsAgreeAtEveryDepthAndForLookupToo)
{
    InMemoryFileSystem fs;
    fs.seedFile("/App/nested.txt", "x");

    const std::vector<std::string> expected{ "nested.txt" };
    EXPECT_EQ(listDir(fs, "App"), expected);
    EXPECT_EQ(listDir(fs, "/App"), expected);
    EXPECT_EQ(listDir(fs, "/App/"), expected);

    EXPECT_TRUE(fs.exist("App/nested.txt"));
    EXPECT_TRUE(fs.exist("/App/nested.txt"));
    EXPECT_EQ(fs.readFile("App/nested.txt"), "x");
}

// A name handed back by a listing must be openable as "/" + name. Every
// directory-scanning caller does exactly this, so if the two disagree a test
// gets a scan that finds names it cannot then read -- silently, because the
// open just fails and the caller takes its not-found branch.
TEST(InMemoryDirectory, AListedNameIsOpenableAsSlashPlusName)
{
    InMemoryFileSystem fs;
    const std::string contents = "seeded without a leading slash";
    fs.seedFile("bare.txt", contents);

    auto dir = fs.dir("/");
    ASSERT_TRUE(dir->open());
    SDK::Interface::IFileSystem::ObjectInfo item{};
    ASSERT_TRUE(dir->readNext(item));
    dir->close();

    const std::string path = std::string("/") + item.name;
    EXPECT_TRUE(fs.exist(path.c_str())) << path << " came out of a listing of \"/\"";
    auto file = fs.file(path.c_str());
    ASSERT_TRUE(file && file->open());
    EXPECT_EQ(file->size(), contents.size());
    file->close();
    EXPECT_EQ(fs.readFile(path), contents);
}

// However a name is arrived at -- two spellings of one file, an implied
// directory that is also an explicit one -- a listing reports it once.
TEST(InMemoryDirectory, AListingNeverReportsTheSameNameTwice)
{
    InMemoryFileSystem fs;
    fs.seedFile("/App/a.txt", "x");
    fs.seedFile("App/b.txt", "y");   // the same directory, spelled differently
    ASSERT_TRUE(fs.mkdir("App"));    // and now explicitly created as well
    fs.seedFile("/solo.txt", "z");
    fs.seedFile("solo.txt", "z");    // the same file, spelled differently

    EXPECT_EQ(listDir(fs, "/"), (std::vector<std::string>{ "App/", "solo.txt" }));
    EXPECT_EQ(listDir(fs, "App"), (std::vector<std::string>{ "a.txt", "b.txt" }));
}

// A documented divergence: "." and ".." are ordinary segments, so a path
// spelled through one is a different place rather than the same one.
TEST(InMemoryDirectory, DotSegmentsAreNotResolved)
{
    InMemoryFileSystem fs;
    fs.seedFile("a/b/c.txt", "x");

    EXPECT_FALSE(fs.exist("a/./b/c.txt"));
    EXPECT_FALSE(fs.exist("a/b/../b/c.txt"));

    // They are stored and enumerated as literal names, not resolved away.
    fs.seedFile("a/./d.txt", "y");
    EXPECT_EQ(listDir(fs, "a"), (std::vector<std::string>{ "./", "b/" }));
    EXPECT_EQ(listDir(fs, "a/."), (std::vector<std::string>{ "d.txt" }));
}

// isHidden is derived from a leading '.' the way the POSIX simulator derives
// it, rather than being left permanently false.
TEST(InMemoryDirectory, DotPrefixedNamesReportAsHidden)
{
    InMemoryFileSystem fs;
    fs.seedFile("d/.hidden", "x");
    fs.seedFile("d/plain.txt", "y");

    auto dir = fs.dir("d");
    ASSERT_TRUE(dir->open());
    SDK::Interface::IFileSystem::ObjectInfo item{};
    ASSERT_TRUE(dir->readNext(item));
    EXPECT_STREQ(item.name, ".hidden");
    EXPECT_TRUE(item.isHidden);
    ASSERT_TRUE(dir->readNext(item));
    EXPECT_STREQ(item.name, "plain.txt");
    EXPECT_FALSE(item.isHidden);
    dir->close();

    ASSERT_TRUE(fs.objectInfo("d/.hidden", item));
    EXPECT_TRUE(item.isHidden);
}

// IDirectory::create() is the simulator's single non-recursive ::mkdir, not
// IFileSystem::mkdir's parents-too behaviour. Pinned because the two sit next
// to each other and quietly differ.
TEST(InMemoryDirectory, CreateDoesNotInventParents)
{
    InMemoryFileSystem fs;
    EXPECT_FALSE(fs.dir("x/y/z")->create()) << "parent \"x/y\" does not exist";
    EXPECT_FALSE(fs.exist("x"));

    EXPECT_TRUE(fs.dir("x")->create());
    EXPECT_TRUE(fs.dir("x/y")->create());
    EXPECT_TRUE(fs.exist("x/y"));

    EXPECT_TRUE(fs.mkdir("p/q/r")) << "IFileSystem::mkdir does create parents";
    EXPECT_TRUE(fs.exist("p/q"));
}

// A name cannot be a file and a directory at once, so mkdir() over an
// existing file fails rather than recording both and leaving the fake able to
// show a listing the device cannot produce.
TEST(InMemoryDirectory, MkdirRefusesANameAFileAlreadyHolds)
{
    InMemoryFileSystem fs;
    fs.seedFile("a.txt", "x");

    EXPECT_FALSE(fs.mkdir("a.txt"));
    EXPECT_FALSE(fs.dir("a.txt")->create());
    EXPECT_FALSE(fs.dir("a.txt")->open()) << "still just a file";
    EXPECT_EQ(listDir(fs, "/"), (std::vector<std::string>{ "a.txt" }));

    // The parents mkdir() creates are subject to the same rule.
    EXPECT_FALSE(fs.mkdir("a.txt/below"));
}

// Repointing an open handle must not keep serving the old directory's entries
// under the new path -- a combination no real backend can show.
TEST(InMemoryDirectory, SetPathInvalidatesAScanInProgress)
{
    InMemoryFileSystem fs;
    fs.seedFile("one/a.txt", "x");
    fs.seedFile("two/b.txt", "y");

    auto dir = fs.dir("one");
    ASSERT_TRUE(dir->open());
    SDK::Interface::IFileSystem::ObjectInfo item{};
    ASSERT_TRUE(dir->readNext(item));
    ASSERT_STREQ(item.name, "a.txt");

    dir->setPath("two");
    EXPECT_FALSE(dir->isOpen()) << "the handle no longer refers to what it opened";
    EXPECT_FALSE(dir->readNext(item)) << "and must not keep serving \"one\"";

    ASSERT_TRUE(dir->open());
    ASSERT_TRUE(dir->readNext(item));
    EXPECT_STREQ(item.name, "b.txt");
}

TEST(InMemoryFileSystem, ANullPathYieldsNoHandleForEitherKind)
{
    InMemoryFileSystem fs;
    EXPECT_EQ(fs.file(nullptr), nullptr);
    EXPECT_EQ(fs.dir(nullptr), nullptr) << "dir() and file() agree on a null path";
}

TEST(InMemoryFileSystem, ExistSeparatesTheRootFromAnEmptyPath)
{
    InMemoryFileSystem fs;
    EXPECT_TRUE(fs.exist("/")) << "the root exists even when nothing is seeded";
    EXPECT_FALSE(fs.exist("")) << "an empty path names nothing, as stat(\"\") does not";
    EXPECT_FALSE(fs.exist(nullptr));

    SDK::Interface::IFileSystem::ObjectInfo item{};
    EXPECT_FALSE(fs.objectInfo("", item));
    EXPECT_FALSE(fs.objectInfo(nullptr, item));
}

TEST(InMemoryDirectory, EnumerationOrderIsSortedAndReproducible)
{
    // The backing store is an unordered_map, so without an explicit ordering
    // anything that picks the "first" matching entry would be a coin flip.
    InMemoryFileSystem fs;
    for (const char* name : { "zulu", "alpha", "mike", "bravo", "yankee" }) {
        fs.seedFile(std::string("d/") + name + ".txt", "x");
    }

    const std::vector<std::string> expected{
        "alpha.txt", "bravo.txt", "mike.txt", "yankee.txt", "zulu.txt"
    };
    EXPECT_EQ(listDir(fs, "d"), expected);
    EXPECT_EQ(listDir(fs, "d"), expected) << "and stable across repeated listings";
}

TEST(InMemoryDirectory, ReportsEntrySize)
{
    InMemoryFileSystem fs;
    fs.seedFile("d/a.txt", std::string(1234, 'x'));

    auto dir = fs.dir("d");
    ASSERT_TRUE(dir->open());
    SDK::Interface::IFileSystem::ObjectInfo item{};
    ASSERT_TRUE(dir->readNext(item));
    EXPECT_STREQ(item.name, "a.txt");
    EXPECT_EQ(item.size, 1234u);
    EXPECT_FALSE(item.isDir);
}

TEST(InMemoryDirectory, MkdirMakesAnEmptyDirectoryEnumerable)
{
    InMemoryFileSystem fs;
    ASSERT_TRUE(fs.mkdir("Debug"));

    EXPECT_TRUE(fs.exist("Debug"));
    EXPECT_EQ(listDir(fs, "/"), (std::vector<std::string>{ "Debug/" }));

    // Assert the open, then the emptiness. An empty listDir() result would
    // also be what a failed open() produces, so on its own it cannot tell an
    // empty directory from an unopenable one.
    auto created = fs.dir("Debug");
    ASSERT_TRUE(created->open()) << "an mkdir'd directory must be openable";
    SDK::Interface::IFileSystem::ObjectInfo item{};
    EXPECT_FALSE(created->readNext(item)) << "created, but empty";
    EXPECT_TRUE(created->close());
}

TEST(InMemoryDirectory, MkdirCreatesParentsAndSucceedsWhenAlreadyPresent)
{
    InMemoryFileSystem fs;
    ASSERT_TRUE(fs.mkdir("a/b/c"));
    EXPECT_TRUE(fs.exist("a"));
    EXPECT_TRUE(fs.exist("a/b"));
    EXPECT_TRUE(fs.exist("a/b/c"));

    // IFileSystem::mkdir is documented as succeeding when the directory
    // already exists, which callers rely on to "just ensure it's there".
    EXPECT_TRUE(fs.mkdir("a/b/c"));

    EXPECT_FALSE(fs.mkdir(nullptr));
}

TEST(InMemoryDirectory, OpeningAMissingDirectoryFails)
{
    InMemoryFileSystem fs;
    auto dir = fs.dir("nope");
    ASSERT_TRUE(dir != nullptr);
    EXPECT_FALSE(dir->open());
    EXPECT_FALSE(dir->exist());
}

TEST(InMemoryDirectory, ReadNextOnAClosedDirectoryFails)
{
    InMemoryFileSystem fs;
    fs.seedFile("d/a.txt", "x");

    auto dir = fs.dir("d");
    SDK::Interface::IFileSystem::ObjectInfo item{};
    EXPECT_FALSE(dir->readNext(item)) << "not open yet";

    ASSERT_TRUE(dir->open());
    ASSERT_TRUE(dir->readNext(item));
    ASSERT_TRUE(dir->close());
    EXPECT_FALSE(dir->readNext(item)) << "closed again";
}

// IDirectory::readNext(reset=true) rewinds WITHOUT reading: item is left
// untouched and the return value reflects only whether the rewind worked.
// Easy to get subtly wrong, and a caller that assumed otherwise would read a
// phantom first entry.
TEST(InMemoryDirectory, ResetRewindsWithoutConsumingAnEntry)
{
    InMemoryFileSystem fs;
    fs.seedFile("d/a.txt", "x");
    fs.seedFile("d/b.txt", "y");

    auto dir = fs.dir("d");
    ASSERT_TRUE(dir->open());

    SDK::Interface::IFileSystem::ObjectInfo item{};
    ASSERT_TRUE(dir->readNext(item));
    ASSERT_STREQ(item.name, "a.txt");
    ASSERT_TRUE(dir->readNext(item));
    ASSERT_STREQ(item.name, "b.txt");
    EXPECT_FALSE(dir->readNext(item)) << "exhausted";

    SDK::Interface::IFileSystem::ObjectInfo untouched{};
    std::strncpy(untouched.name, "SENTINEL", sizeof(untouched.name) - 1);
    EXPECT_TRUE(dir->readNext(untouched, /*reset=*/true));
    EXPECT_STREQ(untouched.name, "SENTINEL") << "a reset must not write into item";

    ASSERT_TRUE(dir->readNext(item));
    EXPECT_STREQ(item.name, "a.txt") << "and the cursor is back at the start";
}

TEST(InMemoryDirectory, SnapshotIsTakenAtOpenNotLive)
{
    InMemoryFileSystem fs;
    fs.seedFile("d/a.txt", "x");

    auto dir = fs.dir("d");
    ASSERT_TRUE(dir->open());

    fs.seedFile("d/b.txt", "y"); // appears after the scan began

    SDK::Interface::IFileSystem::ObjectInfo item{};
    ASSERT_TRUE(dir->readNext(item));
    EXPECT_STREQ(item.name, "a.txt");
    EXPECT_FALSE(dir->readNext(item)) << "mid-scan additions do not appear";

    // ...but an explicit rewind re-snapshots, so they do then.
    ASSERT_TRUE(dir->readNext(item, /*reset=*/true));
    ASSERT_TRUE(dir->readNext(item));
    ASSERT_TRUE(dir->readNext(item));
    EXPECT_STREQ(item.name, "b.txt");
}

// remove() reports whether it actually removed something. A path removed
// earlier still has an entry in the backing map, with its `exists` flag
// cleared, and answering from the entry's presence alone would claim success
// for work not done -- a bad thing to write assertions against.
TEST(InMemoryFileSystem, RemoveReportsWhetherAnythingWasRemoved)
{
    InMemoryFileSystem fs;
    fs.seedFile("a.txt", "x");

    EXPECT_TRUE(fs.remove("a.txt"));
    EXPECT_FALSE(fs.remove("a.txt")) << "already gone";
    EXPECT_FALSE(fs.remove("never-existed.txt"));
    EXPECT_FALSE(fs.remove(nullptr));
}

TEST(InMemoryDirectory, RemovedFilesDisappearFromListings)
{
    InMemoryFileSystem fs;
    fs.seedFile("d/a.txt", "x");
    fs.seedFile("d/b.txt", "y");

    ASSERT_TRUE(fs.remove("d/a.txt"));
    EXPECT_EQ(listDir(fs, "d"), (std::vector<std::string>{ "b.txt" }));
}

TEST(InMemoryDirectory, RemoveRefusesANonEmptyDirectory)
{
    InMemoryFileSystem fs;
    ASSERT_TRUE(fs.mkdir("d"));
    fs.seedFile("d/a.txt", "x");

    EXPECT_FALSE(fs.remove("d")) << "FatFs f_unlink refuses a populated directory";

    ASSERT_TRUE(fs.remove("d/a.txt"));
    EXPECT_TRUE(fs.remove("d")) << "empty now";
    EXPECT_FALSE(fs.exist("d"));
}

// A documented divergence from a real filesystem, pinned so it stays a known
// quantity rather than a surprise: a directory that exists only because a
// file lives under it goes away with that file. mkdir() is what makes one
// outlive its children.
TEST(InMemoryDirectory, AnImpliedDirectoryVanishesWithItsLastChild)
{
    InMemoryFileSystem fs;
    fs.seedFile("implied/a.txt", "x");
    ASSERT_TRUE(fs.exist("implied"));

    ASSERT_TRUE(fs.remove("implied/a.txt"));
    EXPECT_FALSE(fs.exist("implied"))
        << "implied directories are only as durable as their contents";

    InMemoryFileSystem explicitFs;
    ASSERT_TRUE(explicitFs.mkdir("kept"));
    explicitFs.seedFile("kept/a.txt", "x");
    ASSERT_TRUE(explicitFs.remove("kept/a.txt"));
    EXPECT_TRUE(explicitFs.exist("kept")) << "an mkdir'd directory survives";
}

TEST(InMemoryDirectory, ObjectInfoDistinguishesDirectoriesFromFiles)
{
    InMemoryFileSystem fs;
    fs.seedFile("d/a.txt", "hello");

    SDK::Interface::IFileSystem::ObjectInfo item{};
    ASSERT_TRUE(fs.objectInfo("d/a.txt", item));
    EXPECT_FALSE(item.isDir);
    EXPECT_EQ(item.size, 5u);
    // The leaf name, not the path asked about: the same convention the
    // simulator's objectInfo() and readNext() both use.
    EXPECT_STREQ(item.name, "a.txt");

    ASSERT_TRUE(fs.objectInfo("d", item));
    EXPECT_TRUE(item.isDir);
    EXPECT_STREQ(item.name, "d");

    EXPECT_FALSE(fs.objectInfo("d/missing.txt", item));
}

TEST(InMemoryDirectory, DirectoryRenameFailsRatherThanLying)
{
    InMemoryFileSystem fs;
    fs.seedFile("d/a.txt", "x");

    auto dir = fs.dir("d");
    EXPECT_FALSE(dir->rename("e")) << "not modelled; must not silently orphan contents";
    EXPECT_TRUE(fs.exist("d/a.txt"));
}
