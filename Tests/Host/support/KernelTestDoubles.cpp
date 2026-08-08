#include "KernelTestDoubles.hpp"

#include <algorithm>
#include <cstring>
#include <map>
#include <new>

#include "SDK/Messages/MessageBase.hpp"

namespace SDK::TestSupport {

void StubSystem::exit(int status)
{
    lastExitStatus = status;
}

uint32_t StubSystem::getTimeMs()
{
    return nowMs;
}

void StubSystem::delay(uint32_t ms)
{
    nowMs += ms;
}

void StubSystem::yield()
{
}

void StubLogger::printf(const char* format, ...)
{
    (void)format;
}

void StubLogger::vprintf(const char* format, va_list args)
{
    (void)format;
    (void)args;
}

void StubLogger::mvprintf(const char* level, const char* module_name, const char* func, int line, const char* fmt, va_list args)
{
    (void)level;
    (void)module_name;
    (void)func;
    (void)line;
    (void)fmt;
    (void)args;
}

void* StubAppMemory::malloc(size_t size)
{
    return std::malloc(size);
}

void StubAppMemory::free(void* ptr)
{
    std::free(ptr);
}

void* StubAppMemory::realloc(void* ptr, size_t size)
{
    return std::realloc(ptr, size);
}

uint32_t StubAppComm::getProcessId() const
{
    return 1;
}

bool StubAppComm::getMessage(SDK::MessageBase*& msg, uint32_t timeoutMs)
{
    (void)msg;
    (void)timeoutMs;
    return false;
}

void StubAppComm::sendResponse(SDK::MessageBase* msg)
{
    (void)msg;
}

void StubAppComm::releaseMessage(SDK::MessageBase* msg)
{
    if (msg != nullptr) {
        ::operator delete(msg);
    }
}

bool StubAppComm::sendMessage(SDK::MessageBase* msg, uint32_t timeoutMs)
{
    (void)msg;
    (void)timeoutMs;
    return true;
}

void* StubAppComm::allocateMessage(size_t size)
{
    return ::operator new(size, std::nothrow);
}

InMemoryFileSystem::InMemoryFile::InMemoryFile(InMemoryFileSystem& fs, std::string path)
    : mFs(fs)
    , mGivenPath(std::move(path))
    , mPath(canonicalPath(mGivenPath))
{
}

void InMemoryFileSystem::InMemoryFile::setPath(const char* path)
{
    mGivenPath = path != nullptr ? path : "";
    mPath = canonicalPath(mGivenPath);
}

const char* InMemoryFileSystem::InMemoryFile::getPath() const
{
    // The path as the caller spelled it, matching the simulator's File, which
    // hands back exactly what setPath() was given.
    return mGivenPath.c_str();
}

bool InMemoryFileSystem::InMemoryFile::exist() const
{
    auto it = mFs.files.find(mPath);
    return it != mFs.files.end() && it->second.exists;
}

bool InMemoryFileSystem::InMemoryFile::rename(const char* newPath)
{
    if (newPath == nullptr) {
        return false;
    }
    if (!mFs.rename(mPath.c_str(), newPath)) {
        return false;
    }
    // Keep the open-handle instrumentation keyed by the handle's current path:
    // without the migration a rename-while-open leaves the old bucket nonzero
    // forever and the eventual close() would drain the wrong (new) bucket.
    const std::string newCanonical = canonicalPath(newPath);
    if (mOpen) {
        --mFs.openHandles[mPath];
        ++mFs.openHandles[newCanonical];
    }
    mGivenPath = newPath;
    mPath = newCanonical;
    return true;
}

bool InMemoryFileSystem::InMemoryFile::remove()
{
    return mFs.remove(mPath.c_str());
}

size_t InMemoryFileSystem::InMemoryFile::size() const
{
    auto it = mFs.files.find(mPath);
    if (it == mFs.files.end() || !it->second.exists) {
        return 0;
    }
    return it->second.content.size();
}

bool InMemoryFileSystem::InMemoryFile::open(bool wMode, bool override)
{
    // Injected, file-kind-scoped storage failure: refuse to open a matching path
    // for writing without touching the entry (e.g. the auxiliary ".json" summary
    // fails while the ".fit" is already durable).
    if (wMode && !mFs.failWriteOpenSuffix.empty()
        && mPath.size() >= mFs.failWriteOpenSuffix.size()
        && mPath.compare(mPath.size() - mFs.failWriteOpenSuffix.size(),
                         mFs.failWriteOpenSuffix.size(), mFs.failWriteOpenSuffix) == 0) {
        return false;
    }

    mWriteMode = wMode;

    if (wMode) {
        auto& entry = mFs.files[mPath];
        entry.exists = true;
        if (override) {
            entry.content.clear();
        }
    } else {
        auto it = mFs.files.find(mPath);
        if (it == mFs.files.end() || !it->second.exists) {
            return false;
        }
    }

    if (!mOpen) {
        ++mFs.openHandles[mPath];
    }
    mOpen = true;
    mPos = 0;
    return true;
}

bool InMemoryFileSystem::InMemoryFile::isOpen() const
{
    return mOpen;
}

bool InMemoryFileSystem::InMemoryFile::close()
{
    // Injected close failure (FatFs f_close semantics: a sync failure keeps
    // the FIL valid and its lock-table entry held): the handle stays open.
    if (mOpen && mFs.closeGate && !mFs.closeGate(mPath)) {
        return false;
    }
    if (mOpen) {
        --mFs.openHandles[mPath];
    }
    mOpen = false;
    return true;
}

bool InMemoryFileSystem::InMemoryFile::read(char* buff, size_t btr, size_t& br)
{
    if (!mOpen || mWriteMode || buff == nullptr) {
        return false;
    }

    auto it = mFs.files.find(mPath);
    if (it == mFs.files.end() || !it->second.exists) {
        return false;
    }

    const size_t available = it->second.content.size() > mPos ? it->second.content.size() - mPos : 0;
    const size_t toRead = std::min(btr, available);
    if (toRead > 0) {
        std::memcpy(buff, it->second.content.data() + mPos, toRead);
    }
    mPos += toRead;
    br = toRead;
    return true;
}

bool InMemoryFileSystem::InMemoryFile::write(const char* buff, size_t btw, size_t& bw)
{
    if (!mOpen || !mWriteMode || buff == nullptr) {
        return false;
    }

    // Injected storage failure: refuse writes once the byte budget is spent.
    if (mFs.bytesWritten >= mFs.failWritesAfterBytes) {
        return false;
    }

    auto& entry = mFs.files[mPath];
    entry.exists = true;

    if (mPos > entry.content.size()) {
        entry.content.resize(mPos, '\0');
    }

    if (mPos + btw > entry.content.size()) {
        entry.content.resize(mPos + btw);
    }

    std::memcpy(entry.content.data() + mPos, buff, btw);
    mPos += btw;
    bw = btw;
    mFs.bytesWritten += btw;
    return true;
}

bool InMemoryFileSystem::InMemoryFile::seek(size_t offset)
{
    if (!mOpen) {
        return false;
    }
    mPos = offset;
    return true;
}

bool InMemoryFileSystem::InMemoryFile::truncate(size_t offset)
{
    auto it = mFs.files.find(mPath);
    if (!mOpen || it == mFs.files.end() || !it->second.exists) {
        return false;
    }
    it->second.content.resize(offset);
    if (mPos > offset) {
        mPos = offset;
    }
    return true;
}

bool InMemoryFileSystem::InMemoryFile::flush()
{
    if (mOpen) {
        ++mFs.flushCounts[mPath];
    }
    return mOpen;
}

size_t InMemoryFileSystem::InMemoryFile::getPosition() const
{
    return mPos;
}

// -- Path helpers -----------------------------------------------------------
//
// Purely lexical: the backing store is a flat map of canonical paths, so
// "parent" and "base name" are defined by the last '/' and nothing else. No
// "." or ".." resolution, no case folding.

std::string InMemoryFileSystem::canonicalPath(const std::string& path)
{
    size_t begin = 0;
    while (begin < path.size() && path[begin] == '/') {
        ++begin;
    }
    size_t end = path.size();
    while (end > begin && path[end - 1] == '/') {
        --end;
    }
    return path.substr(begin, end - begin);
}

std::string InMemoryFileSystem::parentOf(const std::string& path)
{
    const size_t slash = path.find_last_of('/');
    if (slash == std::string::npos) {
        return {}; // a direct child of the root
    }
    return path.substr(0, slash);
}

std::string InMemoryFileSystem::baseNameOf(const std::string& path)
{
    const size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

bool InMemoryFileSystem::fileExistsAt(const std::string& canonical) const
{
    auto it = files.find(canonical);
    return it != files.end() && it->second.exists;
}

bool InMemoryFileSystem::directoryExists(const std::string& path) const
{
    const std::string dir = canonicalPath(path);
    if (dir.empty()) {
        return true; // the root always exists
    }
    if (directories.count(dir) != 0) {
        return true;
    }
    // Otherwise it exists if some file lives under it. Asking that directly
    // short-circuits on the first hit; materialising every directory in the
    // tree just to answer one question makes every exist() miss cost a walk
    // of the whole store.
    const std::string prefix = dir + "/";
    for (const auto& kv : files) {
        if (kv.second.exists && kv.first.compare(0, prefix.size(), prefix) == 0) {
            return true;
        }
    }
    return false;
}

// -- InMemoryDirectory ------------------------------------------------------

InMemoryFileSystem::InMemoryDirectory::InMemoryDirectory(InMemoryFileSystem& fs, std::string path)
    : mFs(fs), mGivenPath(std::move(path)), mPath(canonicalPath(mGivenPath))
{
}

void InMemoryFileSystem::InMemoryDirectory::setPath(const char* path)
{
    mGivenPath = path != nullptr ? path : "";
    mPath = canonicalPath(mGivenPath);
    // Repointing the handle invalidates any scan in progress: continuing to
    // hand out the previous directory's entries under the new path is a
    // combination no real backend can show. The caller must open() again.
    mOpen = false;
    mEntries.clear();
    mCursor = 0;
}

const char* InMemoryFileSystem::InMemoryDirectory::getPath() const
{
    // The path as the caller spelled it, matching the simulator's Directory.
    return mGivenPath.c_str();
}

bool InMemoryFileSystem::InMemoryDirectory::exist() const
{
    return mFs.directoryExists(mPath);
}

bool InMemoryFileSystem::InMemoryDirectory::rename(const char* newPath)
{
    // Renaming a directory would have to rewrite every descendant's key in
    // the flat map. Nothing needs it, and a rename that silently moved the
    // handle without moving the contents would be worse than an honest
    // failure -- so this fails rather than lies.
    (void)newPath;
    return false;
}

bool InMemoryFileSystem::InMemoryDirectory::remove()
{
    return mFs.remove(mPath.c_str());
}

bool InMemoryFileSystem::InMemoryDirectory::create()
{
    // Deliberately not mkdir(): the simulator's Directory::create() is a
    // single non-recursive ::mkdir that fails with ENOENT when the parent is
    // missing, and a fake that quietly conjures the parents would hide that.
    const std::string parent = parentOf(mPath);
    if (mPath.empty() || mFs.fileExistsAt(mPath) || !mFs.directoryExists(parent)) {
        return false;
    }
    mFs.directories.insert(mPath);
    return true;
}

void InMemoryFileSystem::InMemoryDirectory::snapshot()
{
    mEntries.clear();
    mCursor = 0;

    // Collected by name so a listing can never report the same name twice,
    // however many stored paths or implied directories map onto it.
    std::map<std::string, Entry> byName;
    const std::string prefix = mPath.empty() ? std::string() : mPath + "/";

    for (const auto& kv : mFs.files) {
        if (!kv.second.exists || kv.first.compare(0, prefix.size(), prefix) != 0) {
            continue;
        }
        const std::string rest = kv.first.substr(prefix.size());
        if (rest.empty()) {
            continue;
        }
        const size_t slash = rest.find('/');
        if (slash == std::string::npos) {
            byName[rest] = Entry{ rest, false, kv.second.content.size() };
        } else {
            // A file deeper down: what this directory holds is the segment
            // just below it, reported as a subdirectory.
            const std::string child = rest.substr(0, slash);
            byName.emplace(child, Entry{ child, true, 0 });
        }
    }

    for (const std::string& dir : mFs.directories) {
        if (parentOf(dir) != mPath || dir.empty()) {
            continue;
        }
        const std::string child = baseNameOf(dir);
        byName.emplace(child, Entry{ child, true, 0 });
    }

    // std::map is already ordered by name, which is the reproducible
    // enumeration order the class comment promises.
    mEntries.reserve(byName.size());
    for (auto& kv : byName) {
        mEntries.push_back(std::move(kv.second));
    }
}

bool InMemoryFileSystem::InMemoryDirectory::open()
{
    if (!mFs.directoryExists(mPath)) {
        return false; // opening a directory that isn't there fails, as on device
    }
    mOpen = true;
    snapshot();
    return true;
}

bool InMemoryFileSystem::InMemoryDirectory::isOpen() const
{
    return mOpen;
}

bool InMemoryFileSystem::InMemoryDirectory::readNext(SDK::Interface::IFileSystem::ObjectInfo& item, bool reset)
{
    if (!mOpen) {
        return false;
    }
    if (reset) {
        snapshot();
        return true; // Rewind only -- item is left unmodified, per the interface doc comment.
    }
    if (mCursor >= mEntries.size()) {
        return false;
    }

    const Entry& entry = mEntries[mCursor++];
    // Clipped exactly as the simulator's safe_strcpy would clip it; see the
    // note on name length in the class comment.
    std::strncpy(item.name, entry.name.c_str(), sizeof(item.name) - 1);
    item.name[sizeof(item.name) - 1] = '\0';
    item.isDir    = entry.isDir;
    item.isHidden = entry.name[0] == '.'; // as the POSIX simulator derives it
    item.isSystem = false;
    item.size     = entry.size;
    item.utc      = 0; // Matches InMemoryFileSystem::objectInfo()'s existing convention.
    return true;
}

bool InMemoryFileSystem::InMemoryDirectory::close()
{
    mOpen = false;
    return true;
}

bool InMemoryFileSystem::mkdir(const char* path)
{
    if (path == nullptr) {
        return false;
    }
    // A file anywhere along the chain makes the directory impossible, as on
    // device -- otherwise the fake would hold one name as both a file and an
    // openable directory at once. Checked over the whole chain before
    // anything is recorded, so a rejected mkdir() leaves no half-built path.
    const std::string requested = canonicalPath(path);
    for (std::string dir = requested; !dir.empty(); dir = parentOf(dir)) {
        if (fileExistsAt(dir)) {
            return false;
        }
    }
    // Creates parents too, and succeeds when the directory already exists --
    // both per IFileSystem::mkdir's contract.
    for (std::string dir = requested; !dir.empty(); dir = parentOf(dir)) {
        if (!directories.insert(dir).second) {
            break; // already recorded, so its ancestors are too
        }
    }
    return true;
}

std::unique_ptr<SDK::Interface::IFile> InMemoryFileSystem::file(const char* path)
{
    if (path == nullptr) {
        return {};
    }
    return std::make_unique<InMemoryFile>(*this, path);
}

std::unique_ptr<SDK::Interface::IDirectory> InMemoryFileSystem::dir(const char* path)
{
    if (path == nullptr) {
        return {}; // as file() does: a null path yields no handle
    }
    return std::make_unique<InMemoryDirectory>(*this, path);
}

bool InMemoryFileSystem::exist(const char* path) const
{
    // An empty path names nothing, as stat("") does not. "/" is the root and
    // does exist, and canonicalPath() maps both to "", so separate them here.
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    auto it = files.find(canonicalPath(path));
    if (it != files.end() && it->second.exists) {
        return true;
    }
    // A directory exists too -- IFileSystem::exist() is about filesystem
    // objects, not files specifically.
    return directoryExists(path);
}

bool InMemoryFileSystem::remove(const char* path)
{
    if (path == nullptr) {
        return false;
    }
    const std::string target = canonicalPath(path);
    auto it = files.find(target);
    if (it != files.end() && it->second.exists) {
        it->second.exists = false;
        it->second.content.clear();
        return true;
    }

    // Removing a directory: only an existing, empty one, mirroring FatFs
    // f_unlink, which refuses to remove a directory that still has children.
    if (target.empty() || !directoryExists(target)) {
        return false;
    }
    const std::string prefix = target + "/";
    for (const auto& kv : files) {
        if (kv.second.exists && kv.first.compare(0, prefix.size(), prefix) == 0) {
            return false;
        }
    }
    for (const std::string& other : directories) {
        if (other.compare(0, prefix.size(), prefix) == 0) {
            return false;
        }
    }
    return directories.erase(target) != 0;
}

bool InMemoryFileSystem::rename(const char* oldPath, const char* newPath)
{
    if (oldPath == nullptr || newPath == nullptr) {
        return false;
    }
    auto it = files.find(canonicalPath(oldPath));
    if (it == files.end()) {
        return false;
    }
    files[canonicalPath(newPath)] = std::move(it->second);
    files.erase(it);
    return true;
}

bool InMemoryFileSystem::copy(const char* oldPath, const char* newPath)
{
    if (oldPath == nullptr || newPath == nullptr) {
        return false;
    }
    auto it = files.find(canonicalPath(oldPath));
    if (it == files.end()) {
        return false;
    }
    files[canonicalPath(newPath)] = it->second;
    return true;
}

bool InMemoryFileSystem::objectInfo(const char* path, ObjectInfo& item) const
{
    // As with exist(): an empty path is not an object, the root is.
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    const std::string target = canonicalPath(path);

    // The leaf name, not the path handed in -- the convention the simulator's
    // objectInfo() follows, and the one every readNext() already used.
    const std::string name = baseNameOf(target);
    const auto populate = [&item, &name](bool isDir, size_t size) {
        std::strncpy(item.name, name.c_str(), sizeof(item.name) - 1);
        item.name[sizeof(item.name) - 1] = '\0';
        item.isDir = isDir;
        item.isHidden = !name.empty() && name[0] == '.';
        item.isSystem = false;
        item.size = size;
        item.utc = 0;
    };

    auto it = files.find(target);
    if (it != files.end() && it->second.exists) {
        populate(false, it->second.content.size());
        return true;
    }

    if (!directoryExists(target)) {
        return false;
    }
    populate(true, 0);
    return true;
}

void InMemoryFileSystem::seedFile(const std::string& path, std::string content)
{
    files[canonicalPath(path)] = InMemoryFileEntry{ std::move(content), true };
}

std::string InMemoryFileSystem::readFile(const std::string& path) const
{
    auto it = files.find(canonicalPath(path));
    if (it == files.end() || !it->second.exists) {
        return {};
    }
    return it->second.content;
}

KernelFixture::KernelFixture()
    : system()
    , logger()
    , memory()
    , comm()
    , fileSystem()
    , kernel(system, logger, memory, comm, fileSystem)
{
}

} // namespace SDK::TestSupport
