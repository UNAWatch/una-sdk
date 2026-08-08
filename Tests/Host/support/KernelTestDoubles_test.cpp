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

// rename() may keep a handle open, being the same file under a new name, and
// migrates the count with it. setPath() names a DIFFERENT file, so it closes
// instead: a handle carried across the change would release a bucket it never
// took, underflowing size_t and taking the leak instrumentation with it.
TEST(InMemoryFileSystem, SetPathOnAnOpenFileClosesItRatherThanRetargeting)
{
    InMemoryFileSystem fs;
    fs.seedFile("a.txt", "AAA");
    fs.seedFile("b.txt", "BBB");

    auto f = fs.file("a.txt");
    ASSERT_TRUE(f->open(/*wMode=*/true));
    ASSERT_EQ(fs.openHandles["a.txt"], 1u);

    f->setPath("b.txt");
    EXPECT_FALSE(f->isOpen()) << "the handle no longer refers to what it opened";
    EXPECT_EQ(fs.openHandles["a.txt"], 0u) << "the old bucket is released, not stranded";
    EXPECT_EQ(fs.openHandles["b.txt"], 0u) << "and the new path was never opened";

    size_t bw = 0;
    EXPECT_FALSE(f->write("ZZ", 2, bw)) << "a closed handle cannot write";
    EXPECT_EQ(fs.readFile("b.txt"), "BBB") << "b.txt must not be touched";
    EXPECT_EQ(fs.readFile("a.txt"), "AAA");

    EXPECT_FALSE(f->close()) << "already closed";
    EXPECT_EQ(fs.openHandles["b.txt"], 0u) << "no size_t underflow to SIZE_MAX";
    EXPECT_EQ(fs.openHandles["a.txt"], 0u);
}

// close() reports whether it closed something, so EXPECT_TRUE(h->close())
// has a failing mode: a handle never opened, or one naming a path that does
// not exist, answers false. The simulator's File::close() and
// Directory::close() both do.
TEST(InMemoryFileSystem, ClosingWhatWasNeverOpenedFails)
{
    InMemoryFileSystem fs;
    fs.seedFile("a.txt", "x");
    ASSERT_TRUE(fs.mkdir("d"));

    EXPECT_FALSE(fs.file("a.txt")->close()) << "never opened";
    EXPECT_FALSE(fs.file("never-seeded.txt")->close());
    EXPECT_FALSE(fs.dir("d")->close()) << "never opened";
    EXPECT_FALSE(fs.dir("does-not-exist")->close());

    auto f = fs.file("a.txt");
    ASSERT_TRUE(f->open());
    EXPECT_TRUE(f->close()) << "an open handle closes";
    EXPECT_FALSE(f->close()) << "but not twice";

    auto d = fs.dir("d");
    ASSERT_TRUE(d->open());
    EXPECT_TRUE(d->close());
    EXPECT_FALSE(d->close()) << "but not twice";
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

// Both listing guarantees -- a name joins back onto its parent, and a name
// appears once -- hold up to ObjectInfo::name and stop there. A name too long
// to survive the copy comes back clipped, so it names nothing, and two names
// differing only past that capacity come back as one repeated entry. Pinned
// because the documented rules are stated with this boundary attached, and a
// test that scans and then opens is the pattern that meets it.
TEST(InMemoryDirectory, ListingGuaranteesStopAtTheNameCapacity)
{
    constexpr size_t kCap = SDK::Interface::IFileSystem::skMaxPathLen; // incl. NUL
    InMemoryFileSystem fs;
    const std::string fits(kCap - 1, 'a');       // 255: survives intact
    const std::string tooLong(kCap, 'b');        // 256: clipped to 255
    fs.seedFile("d/" + fits, "x");
    fs.seedFile("d/" + tooLong, "y");

    auto dir = fs.dir("d");
    ASSERT_TRUE(dir->open());
    SDK::Interface::IFileSystem::ObjectInfo item{};
    size_t joinable = 0;
    size_t clipped = 0;
    while (dir->readNext(item)) {
        const std::string name(item.name);
        EXPECT_EQ(name.size(), kCap - 1) << "both clip or fit at the capacity";
        if (fs.exist(("d/" + name).c_str())) {
            ++joinable;
        } else {
            ++clipped;
        }
    }
    ASSERT_TRUE(dir->close());
    EXPECT_EQ(joinable, 1u) << "the name that fits joins back onto its parent";
    EXPECT_EQ(clipped, 1u) << "the one that does not names nothing";

    // Two names differing only past the capacity: one name, reported twice.
    InMemoryFileSystem twins;
    twins.seedFile("d/" + std::string(kCap, 'c') + "ONE", "1");
    twins.seedFile("d/" + std::string(kCap, 'c') + "TWO", "2");
    auto twinDir = twins.dir("d");
    ASSERT_TRUE(twinDir->open());
    std::vector<std::string> seen;
    while (twinDir->readNext(item)) {
        seen.push_back(item.name);
    }
    ASSERT_TRUE(twinDir->close());
    ASSERT_EQ(seen.size(), 2u);
    EXPECT_EQ(seen[0], seen[1]) << "the one case where a listing repeats a name";
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

    // The root is already a directory, which ::mkdir reports as EEXIST and
    // the simulator turns into success once it confirms the target is one.
    EXPECT_TRUE(fs.dir("/")->create()) << "the root exists, so creating it succeeds";
    EXPECT_TRUE(fs.dir("")->create());
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

// One name is never both a file and a directory, whichever way it is
// approached: mkdir() and create() refuse a name a file holds, a write-open
// gets EISDIR on the real backend, and seedFile() cannot plant what IFile
// cannot open.
TEST(InMemoryDirectory, AFileCannotShadowADirectory)
{
    InMemoryFileSystem fs;
    ASSERT_TRUE(fs.mkdir("d"));
    fs.seedFile("d/inner.txt", "child");

    EXPECT_FALSE(fs.file("d")->open(/*wMode=*/true)) << "EISDIR: \"d\" is a directory";
    EXPECT_FALSE(fs.seedFile("d", "clobber")) << "and seeding cannot do it either";

    // The directory is intact, and nothing reports "d" as a file.
    EXPECT_EQ(listDir(fs, "/"), (std::vector<std::string>{ "d/" }));
    EXPECT_EQ(listDir(fs, "d"), (std::vector<std::string>{ "inner.txt" }));
    SDK::Interface::IFileSystem::ObjectInfo item{};
    ASSERT_TRUE(fs.objectInfo("d", item));
    EXPECT_TRUE(item.isDir);

    // An implied directory holds the name just as firmly as an mkdir'd one.
    InMemoryFileSystem implied;
    implied.seedFile("imp/child.txt", "x");
    EXPECT_FALSE(implied.file("imp")->open(/*wMode=*/true));
    EXPECT_FALSE(implied.seedFile("imp", "clobber"));

    // ...and the root is a directory too.
    EXPECT_FALSE(fs.file("/")->open(/*wMode=*/true));
}

// The mirror image: nothing may live under a name a file already holds.
// mkdir() and create() already refuse; a write-open and seedFile() must too.
TEST(InMemoryDirectory, NothingCanBeCreatedUnderAFile)
{
    InMemoryFileSystem fs;
    fs.seedFile("a.txt", "x");

    EXPECT_FALSE(fs.mkdir("a.txt/below"));
    EXPECT_FALSE(fs.dir("a.txt/below")->create());
    EXPECT_FALSE(fs.file("a.txt/below/deep.bin")->open(/*wMode=*/true))
        << "ENOTDIR: an ancestor is a file";
    EXPECT_FALSE(fs.seedFile("a.txt/below/deep.bin", "y"));

    EXPECT_EQ(fs.readFile("a.txt"), "x") << "the file is untouched";
    EXPECT_FALSE(fs.dir("a.txt")->open()) << "and never became a directory";
    EXPECT_EQ(listDir(fs, "/"), (std::vector<std::string>{ "a.txt" }));
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
    EXPECT_FALSE(fs.exist(""))
        << "an empty path names nothing, as FatFs f_stat(\"\") is FR_INVALID_NAME";
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

// rename()/copy() follow the same rule remove() does: an entry whose `exists`
// flag is cleared is a tombstone, not a file. Taking one as a source would
// claim success for work not done, and would carry the tombstone onto the
// destination, destroying a live file there.
TEST(InMemoryFileSystem, RenameAndCopyRefuseAnAlreadyRemovedSource)
{
    InMemoryFileSystem fs;
    fs.seedFile("gone.txt", "x");
    fs.seedFile("live.txt", "IMPORTANT");
    ASSERT_TRUE(fs.remove("gone.txt"));

    EXPECT_FALSE(fs.rename("gone.txt", "live.txt")) << "a tombstone is not a source";
    EXPECT_EQ(fs.readFile("live.txt"), "IMPORTANT") << "and must not destroy the destination";
    EXPECT_TRUE(fs.exist("live.txt"));

    EXPECT_FALSE(fs.copy("gone.txt", "c.txt"));
    EXPECT_FALSE(fs.exist("c.txt")) << "no tombstone propagated to the destination";

    // A path that never existed at all already failed; the two now agree.
    EXPECT_FALSE(fs.rename("never.txt", "x.txt"));
    EXPECT_FALSE(fs.copy("never.txt", "x.txt"));
}

// A destination that cannot hold a file is refused rather than silently
// swallowing the source: the root, an existing directory, or a path under a
// file. An existing *file* is still overwritten, as std::rename does.
TEST(InMemoryFileSystem, RenameAndCopyRefuseADestinationThatCannotHoldAFile)
{
    InMemoryFileSystem fs;
    fs.seedFile("src.txt", "SRC");
    ASSERT_TRUE(fs.mkdir("d"));
    fs.seedFile("blocker.txt", "B");

    EXPECT_FALSE(fs.rename("src.txt", "")) << "the root is not a file destination";
    EXPECT_FALSE(fs.rename("src.txt", "/"));
    EXPECT_FALSE(fs.rename("src.txt", "d")) << "an existing directory";
    EXPECT_FALSE(fs.rename("src.txt", "blocker.txt/under")) << "under a file";
    EXPECT_FALSE(fs.copy("src.txt", ""));
    EXPECT_FALSE(fs.copy("src.txt", "d"));
    EXPECT_EQ(fs.readFile("src.txt"), "SRC") << "every refusal left the source alone";
    EXPECT_EQ(listDir(fs, "/"),
              (std::vector<std::string>{ "blocker.txt", "d/", "src.txt" }));

    // Overwriting a plain file is still allowed, as std::rename does it.
    EXPECT_TRUE(fs.rename("src.txt", "blocker.txt"));
    EXPECT_EQ(fs.readFile("blocker.txt"), "SRC");
    EXPECT_FALSE(fs.exist("src.txt"));
}

// Source and destination can name one object through two spellings, which is
// one key in the backing map. Moving an entry onto itself would leave it
// unspecified and then erase it, so the call would report success and take
// the file with it. std::rename succeeds and leaves the file alone.
TEST(InMemoryFileSystem, RenamingAFileOntoItselfKeepsIt)
{
    for (const char* destination : { "a.txt", "/a.txt", "a.txt/", "/a.txt/" }) {
        InMemoryFileSystem fs;
        fs.seedFile("a.txt", "PAYLOAD");

        EXPECT_TRUE(fs.rename("a.txt", destination)) << destination;
        EXPECT_TRUE(fs.exist("a.txt")) << destination << " must not remove the file";
        EXPECT_EQ(fs.readFile("a.txt"), "PAYLOAD") << destination;
        EXPECT_EQ(listDir(fs, "/"), (std::vector<std::string>{ "a.txt" })) << destination;
    }

    // The same key reached through a collapsed separator, at depth.
    InMemoryFileSystem nested;
    nested.seedFile("d/a.txt", "PAYLOAD");
    EXPECT_TRUE(nested.rename("d/a.txt", "d//a.txt"));
    EXPECT_EQ(nested.readFile("d/a.txt"), "PAYLOAD");

    // copy() has the same one-key case. Its contract is that the file is left
    // as it was -- deliberately unlike the simulator, which truncates the
    // destination before reading the source and so empties it. Nothing about
    // the current implementation can violate this, since a self-copy-assign
    // is a no-op; the assertion is here to pin the contract, not to guard it.
    InMemoryFileSystem copied;
    copied.seedFile("a.txt", "PAYLOAD");
    EXPECT_TRUE(copied.copy("a.txt", "/a.txt"));
    EXPECT_EQ(copied.readFile("a.txt"), "PAYLOAD");
}

// Renaming across a rehash of the backing map. Padded to the load-factor
// boundary so the destination insert is guaranteed to rehash rather than
// merely able to.
//
// This pins the outcome, not the mechanism. rename() must erase by key rather
// than through the iterator that found the source, because a rehash
// invalidates every iterator into an unordered_map -- but libstdc++ keeps its
// nodes across a rehash, so erasing through the stale iterator still happens
// to work here and no assertion below can see the difference. The rule is
// upheld by construction; treat this as documentation of the path, not as its
// guard.
TEST(InMemoryFileSystem, RenameMovesAnEntryAcrossARehash)
{
    InMemoryFileSystem fs;
    fs.files.max_load_factor(1.0f);
    fs.seedFile("src.txt", "PAYLOAD");
    for (size_t n = 0; fs.files.size() < fs.files.bucket_count(); ++n) {
        fs.seedFile("pad/f" + std::to_string(n) + ".txt", "x");
    }
    const size_t bucketsBefore = fs.files.bucket_count();
    const size_t sizeBefore = fs.files.size();

    ASSERT_TRUE(fs.rename("src.txt", "dst.txt"));

    EXPECT_GT(fs.files.bucket_count(), bucketsBefore) << "the insert did rehash";
    EXPECT_EQ(fs.files.size(), sizeBefore) << "one entry moved, none gained or lost";
    EXPECT_EQ(fs.readFile("dst.txt"), "PAYLOAD");
    EXPECT_FALSE(fs.exist("src.txt"));
}

// Repeated interior separators are not a divergence: "a//b" and "a/b" are one
// object, as on POSIX and FatFs. Splitting them apart would leave an empty
// path segment, which enumerates as an entry with no name -- something no
// backend can show, and which no caller could join back onto its parent.
TEST(InMemoryDirectory, RepeatedSeparatorsCollapse)
{
    InMemoryFileSystem fs;
    fs.seedFile("a//b.txt", "x");

    EXPECT_TRUE(fs.exist("a//b.txt"));
    EXPECT_TRUE(fs.exist("a/b.txt")) << "one object, however it is spelled";
    EXPECT_EQ(fs.readFile("a/b.txt"), "x");
    EXPECT_EQ(listDir(fs, "a"), (std::vector<std::string>{ "b.txt" }));
    EXPECT_EQ(listDir(fs, "a//"), (std::vector<std::string>{ "b.txt" }));

    // Every listed name still joins back onto its parent.
    auto dir = fs.dir("a");
    ASSERT_TRUE(dir->open());
    SDK::Interface::IFileSystem::ObjectInfo item{};
    ASSERT_TRUE(dir->readNext(item));
    ASSERT_TRUE(dir->close());
    ASSERT_GT(std::strlen(item.name), 0u) << "a listing never yields an empty name";
    const std::string joined = std::string("a/") + item.name;
    EXPECT_TRUE(fs.exist(joined.c_str())) << joined;
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
