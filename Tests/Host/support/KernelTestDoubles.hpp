#pragma once

#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Interfaces/IAppComm.hpp"
#include "SDK/Interfaces/IAppMemory.hpp"
#include "SDK/Interfaces/IFileSystem.hpp"
#include "SDK/Interfaces/ILogger.hpp"
#include "SDK/Interfaces/ISystem.hpp"

namespace SDK::TestSupport {

class StubSystem : public SDK::Interface::ISystem {
public:
    void exit(int status = 0) override;
    uint32_t getTimeMs() override;
    void delay(uint32_t ms) override;
    void yield() override;

    int lastExitStatus = 0;
    uint32_t nowMs = 0;
};

class StubLogger : public SDK::Interface::ILogger {
public:
    void printf(const char *format, ...) override;
    void vprintf(const char *format, va_list args) override;
    void mvprintf(const char *level, const char *module_name, const char *func, int line, const char *fmt, va_list args) override;
};

class StubAppMemory : public SDK::Interface::IAppMemory {
public:
    void* malloc(size_t size) override;
    void free(void* ptr) override;
    void* realloc(void* ptr, size_t size) override;
};

class StubAppComm : public SDK::Interface::IAppComm {
public:
    uint32_t getProcessId() const override;
    bool getMessage(SDK::MessageBase*& msg, uint32_t timeoutMs = 0xFFFFFFFF) override;
    void sendResponse(SDK::MessageBase* msg) override;
    void releaseMessage(SDK::MessageBase* msg) override;
    bool sendMessage(SDK::MessageBase* msg, uint32_t timeoutMs = 0) override;

protected:
    void* allocateMessage(size_t size) override;
};

struct InMemoryFileEntry {
    std::string content;
    bool exists = false;
};

class InMemoryFileSystem : public SDK::Interface::IFileSystem {
public:
    class InMemoryFile : public SDK::Interface::IFile {
    public:
        InMemoryFile(InMemoryFileSystem& fs, std::string path);
        ~InMemoryFile() override = default;

        void setPath(const char* path) override;
        const char* getPath() const override;
        bool exist() const override;
        bool rename(const char* newPath) override;
        bool remove() override;

        size_t size() const override;
        bool open(bool wMode = false, bool override = false) override;
        bool isOpen() const override;
        bool close() override;
        bool read(char* buff, size_t btr, size_t& br) override;
        bool write(const char* buff, size_t btw, size_t& bw) override;
        bool seek(size_t offset) override;
        bool truncate(size_t offset) override;
        bool flush() override;
        size_t getPosition() const override;

    private:
        InMemoryFileSystem& mFs;
        std::string mGivenPath; ///< As handed to the ctor/setPath, for getPath().
        std::string mPath;      ///< Canonical form, used for lookups.
        bool mOpen = false;
        bool mWriteMode = false;
        size_t mPos = 0;
    };

    /**
     * @brief Enumerating directory over the flat `files` map plus any
     *        explicitly created directories.
     *
     * A path's parent is everything before its last '/', so "Activity/a.fit"
     * is a child of "Activity" and "a.fit" is a child of the root. Directories
     * are those created through mkdir()/create() *and* those implied by a
     * file's path: seeding "a/b/c.txt" makes "a" and "a/b" enumerable
     * without any mkdir call. Both files and subdirectories are reported,
     * with @c isDir set accordingly.
     *
     * Enumeration order is **sorted by name**, and deliberately so: the
     * backing store is an unordered_map, so anything depending on
     * enumeration order (picking the "first" matching entry, say) would
     * otherwise be a coin flip from run to run. Real FAT enumerates in
     * directory-entry order, so do not read this ordering as a guarantee
     * the device makes -- it is a guarantee the *fake* makes, so that tests
     * are reproducible.
     *
     * A listing never reports the same name twice, whatever mix of spellings
     * and implied/explicit directories produced it.
     *
     * readNext() is a cursor over a snapshot taken at open() (and re-taken
     * on an explicit reset=true, matching POSIX rewinddir, which refreshes
     * the stream to the directory's current state). Entries added mid-scan
     * do not retroactively appear until such a rewind.
     *
     * Known divergences from a real backend, all of them deliberate. Check
     * these before writing a test that leans on one:
     *
     *  - **An implied directory is only as durable as its contents.** A
     *    directory that exists solely because a file lives under it stops
     *    existing when that file is removed; a real filesystem would keep
     *    the now-empty directory. mkdir() it if a test needs it to outlive
     *    its children.
     *  - **Leading and trailing slashes are not significant.** "/a.txt",
     *    "a.txt" and "a.txt/" all name one object, at every depth, for
     *    enumeration *and* for lookup alike. This is what lets a test seed
     *    "/App.uapp" and have code that scans "/" and opens "/" + name find
     *    it. A real filesystem would treat a trailing slash as requiring a
     *    directory.
     *  - **No "." / ".." resolution.** Both are ordinary path segments, so
     *    "a/b" and "a/./b" are different places.
     *  - **Directory rename is not modelled** -- it returns false rather
     *    than pretending to succeed. The simulator's Directory::rename()
     *    does work, so do not read a false here as device behaviour.
     *  - **No hidden/system attributes of their own.** isHidden is derived
     *    from a leading '.' in the name, as the POSIX simulator does;
     *    isSystem is always false.
     *  - **A name longer than ObjectInfo::name cannot round-trip.** It is
     *    reported clipped, exactly as the simulator's safe_strcpy would clip
     *    it. Real backends cap a single name well below that, so seeding one
     *    that long is a test-authoring mistake rather than a case to model.
     */
    class InMemoryDirectory : public SDK::Interface::IDirectory {
    public:
        InMemoryDirectory(InMemoryFileSystem& fs, std::string path);
        ~InMemoryDirectory() override = default;

        /// Repoints the handle. Any scan in progress is invalidated: the
        /// handle closes and must be open()ed again before readNext().
        void setPath(const char* path) override;
        const char* getPath() const override;
        bool exist() const override;
        /// Always false: directory rename is not modelled by this fake.
        bool rename(const char* newPath) override;
        bool remove() override;

        /// Creates this one directory, and fails when its parent is missing
        /// or a file already occupies the name -- as the simulator's
        /// non-recursive ::mkdir does. Use IFileSystem::mkdir() for the
        /// parents-too behaviour.
        bool create() override;
        bool open() override;
        bool isOpen() const override;
        bool readNext(SDK::Interface::IFileSystem::ObjectInfo& item, bool reset = false) override;
        bool close() override;

    private:
        struct Entry {
            std::string name;
            bool        isDir = false;
            size_t      size  = 0;
        };

        InMemoryFileSystem& mFs;
        std::string         mGivenPath; ///< As handed to the ctor/setPath, for getPath().
        std::string         mPath;      ///< Canonical form, used for lookups.
        bool                mOpen = false;
        std::vector<Entry>  mEntries; // sorted, snapshotted at open()/reset
        size_t              mCursor = 0;

        void snapshot();
    };

    /// Creates the directory and any missing parents; true if it already
    /// existed, per IFileSystem::mkdir. False for a null path, and false when
    /// a file already occupies the name.
    bool mkdir(const char* path) override;
    std::unique_ptr<SDK::Interface::IFile> file(const char* path) override;
    std::unique_ptr<SDK::Interface::IDirectory> dir(const char* path) override;
    /// True for an existing file OR directory: IFileSystem::exist() asks
    /// about filesystem objects, not files specifically. An empty path names
    /// nothing and is false, as stat("") is; "/" is the root and is true.
    bool exist(const char* path) const override;
    /// Removes an existing file, or an existing and empty directory.
    ///
    /// Returns false when there is nothing there to remove, a path removed
    /// earlier included: such an entry lingers in the backing map with its
    /// `exists` flag cleared, and reporting success for work not done is a
    /// bad thing to write assertions against.
    bool remove(const char* path) override;
    bool rename(const char* oldPath, const char* newPath) override;
    bool copy(const char* oldPath, const char* newPath) override;
    /// Populates @p item for an existing file or directory. @c name receives
    /// the leaf name, not the path handed in -- the same convention the
    /// simulator's objectInfo() and every readNext() follow.
    bool objectInfo(const char* path, ObjectInfo& item) const override;

    void seedFile(const std::string& path, std::string content);
    std::string readFile(const std::string& path) const;

    /// Canonical lookup key for @p path: leading and trailing '/' stripped,
    /// so "/a/b", "a/b" and "a/b/" are one object and the root is "". This is
    /// applied at every entry point, which is what makes a name returned by a
    /// listing openable as "/" + name. No "." / ".." resolution.
    static std::string canonicalPath(const std::string& path);
    /// Directory of a canonical @p path: everything before its last '/', or
    /// "" (the root) when it has none.
    static std::string parentOf(const std::string& path);
    /// Final path segment of @p path.
    static std::string baseNameOf(const std::string& path);

    /// True when an existing file sits at @p canonical (already canonicalised).
    bool fileExistsAt(const std::string& canonical) const;
    /// True when @p path is the root, was created via mkdir(), or is implied
    /// by an existing file living under it.
    bool directoryExists(const std::string& path) const;

    /// Keyed by canonicalPath(); seedFile() and every IFileSystem entry point
    /// canonicalise before touching this, so a test that reaches in directly
    /// must use the canonical spelling ("d/a.txt", never "/d/a.txt").
    std::unordered_map<std::string, InMemoryFileEntry> files;

    /// Directories created explicitly through mkdir()/IDirectory::create(),
    /// canonicalised and without the root. Directories implied by a file's
    /// path are NOT stored here -- see directoryExists(), which considers
    /// both.
    std::set<std::string> directories;

    // -- Test instrumentation -------------------------------------------------
    /// Number of successful flush() calls, keyed by file path.
    std::unordered_map<std::string, size_t> flushCounts;
    /// Currently-open handle count, keyed by file path: a successful open()
    /// increments, close() of an open handle decrements. Destroying a handle
    /// does NOT decrement (mirrors IFile: destructors do not close), so tests
    /// can assert no handle -- and thus no FatFs lock slot -- is leaked.
    std::unordered_map<std::string, size_t> openHandles;
    /// Total bytes written across all files (successful writes only).
    size_t bytesWritten = 0;
    /// Once bytesWritten reaches this threshold, subsequent write() calls fail
    /// (simulates a storage write error). Defaults to "never fail".
    size_t failWritesAfterBytes = static_cast<size_t>(-1);
    /// A write-mode open() targeting a path ending with this suffix fails without
    /// creating/touching the entry (simulates a storage error scoped to one file
    /// kind, e.g. the auxiliary ".json" summary). Empty = never fail.
    std::string failWriteOpenSuffix;
    /// Fault hook: when set, consulted before an open handle's close() with the
    /// file path; returning false makes that close() fail and leaves the handle
    /// OPEN (FatFs f_close semantics: a sync failure keeps the FIL valid and
    /// its lock-table entry held). Unset = closes never fail.
    std::function<bool(const std::string& path)> closeGate;
};

struct KernelFixture {
    StubSystem system;
    StubLogger logger;
    StubAppMemory memory;
    StubAppComm comm;
    InMemoryFileSystem fileSystem;
    SDK::Kernel kernel;

    KernelFixture();
};

} // namespace SDK::TestSupport
