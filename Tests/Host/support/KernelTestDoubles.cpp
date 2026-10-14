#include "KernelTestDoubles.hpp"

#include <algorithm>
#include <cstring>
#include <map>
#include <new>

#include "SDK/Messages/MessageBase.hpp"

namespace SDK::TestSupport {

namespace {

bool isRoot(const std::string& canonical)
{
    return canonical.empty();
}

} // namespace

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
    , mCanonicalPath(canonicalPath(mGivenPath))
{
}

void InMemoryFileSystem::InMemoryFile::setPath(const char* path)
{
    if (mOpen) {
        --mFs.openHandles[mCanonicalPath];
    }
    mOpen = false;
    mWriteMode = false;
    mPos = 0;
    mGivenPath = path != nullptr ? path : "";
    mCanonicalPath = canonicalPath(mGivenPath);
}

const char* InMemoryFileSystem::InMemoryFile::getPath() const
{
    return mGivenPath.c_str();
}

bool InMemoryFileSystem::InMemoryFile::exist() const
{
    auto it = mFs.files.find(mCanonicalPath);
    return it != mFs.files.end() && it->second.exists;
}

bool InMemoryFileSystem::InMemoryFile::rename(const char* newPath)
{
    if (newPath == nullptr) {
        return false;
    }
    if (!mFs.rename(mCanonicalPath.c_str(), newPath)) {
        return false;
    }
    const std::string newCanonical = canonicalPath(newPath);
    // Keep the open-handle instrumentation keyed by the handle's current path:
    // without the migration a rename-while-open leaves the old bucket nonzero
    // forever and the eventual close() would drain the wrong (new) bucket.
    if (mOpen) {
        --mFs.openHandles[mCanonicalPath];
        ++mFs.openHandles[newCanonical];
    }
    mGivenPath = newPath;
    mCanonicalPath = newCanonical;
    return true;
}

bool InMemoryFileSystem::InMemoryFile::remove()
{
    return mFs.remove(mCanonicalPath.c_str());
}

size_t InMemoryFileSystem::InMemoryFile::size() const
{
    auto it = mFs.files.find(mCanonicalPath);
    if (it == mFs.files.end() || !it->second.exists) {
        return 0;
    }
    return it->second.content.size();
}

bool InMemoryFileSystem::InMemoryFile::open(bool wMode, bool override)
{
    if (wMode && !mFs.canHoldFileAt(mCanonicalPath)) {
        return false;
    }

    // Injected, file-kind-scoped storage failure: refuse to open a matching path
    // for writing without touching the entry (e.g. the auxiliary ".json" summary
    // fails while the ".fit" is already durable).
    if (wMode && !mFs.failWriteOpenSuffix.empty()
        && mCanonicalPath.size() >= mFs.failWriteOpenSuffix.size()
        && mCanonicalPath.compare(mCanonicalPath.size() - mFs.failWriteOpenSuffix.size(),
                                  mFs.failWriteOpenSuffix.size(), mFs.failWriteOpenSuffix) == 0) {
        return false;
    }

    mWriteMode = wMode;

    if (wMode) {
        auto& entry = mFs.files[mCanonicalPath];
        if (!entry.exists) {
            entry.exists = true;
            entry.created = mFs.nextCreated();
        }
        if (override) {
            entry.content.clear();
        }
    } else {
        auto it = mFs.files.find(mCanonicalPath);
        if (it == mFs.files.end() || !it->second.exists) {
            return false;
        }
    }

    if (!mOpen) {
        ++mFs.openHandles[mCanonicalPath];
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
    if (!mOpen) {
        return false;
    }
    // Injected close failure (FatFs f_close semantics: a sync failure keeps
    // the FIL valid and its lock-table entry held): the handle stays open.
    if (mFs.closeGate && !mFs.closeGate(mCanonicalPath)) {
        return false;
    }
    --mFs.openHandles[mCanonicalPath];
    mOpen = false;
    return true;
}

bool InMemoryFileSystem::InMemoryFile::read(char* buff, size_t btr, size_t& br)
{
    if (!mOpen || mWriteMode || buff == nullptr) {
        return false;
    }

    auto it = mFs.files.find(mCanonicalPath);
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

    if (!mFs.fileExistsAt(mCanonicalPath) && !mFs.canHoldFileAt(mCanonicalPath)) {
        return false;
    }
    auto& entry = mFs.files[mCanonicalPath];
    if (!entry.exists) {
        entry.exists = true;
        entry.created = mFs.nextCreated();
    }

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
    auto it = mFs.files.find(mCanonicalPath);
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
        ++mFs.flushCounts[mCanonicalPath];
    }
    return mOpen;
}

size_t InMemoryFileSystem::InMemoryFile::getPosition() const
{
    return mPos;
}

std::string InMemoryFileSystem::canonicalPath(const std::string& path)
{
    std::string out;
    out.reserve(path.size());
    for (const char c : path) {
        if (c == '/' && (out.empty() || out.back() == '/')) {
            continue;
        }
        out.push_back(c);
    }
    while (!out.empty() && out.back() == '/') {
        out.pop_back();
    }
    return out;
}

std::string InMemoryFileSystem::parentOf(const std::string& path)
{
    const size_t slash = path.find_last_of('/');
    if (slash == std::string::npos) {
        return {};
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

bool InMemoryFileSystem::canHoldFileAt(const std::string& canonical) const
{
    if (isRoot(canonical)) {
        return false;
    }
    if (directoryExists(canonical)) {
        return false;
    }
    for (std::string dir = parentOf(canonical); !isRoot(dir); dir = parentOf(dir)) {
        if (fileExistsAt(dir)) {
            return false;
        }
    }
    return true;
}

bool InMemoryFileSystem::directoryExists(const std::string& path) const
{
    const std::string dir = canonicalPath(path);
    if (isRoot(dir)) {
        return true;
    }
    if (directories.count(dir) != 0) {
        return true;
    }
    const std::string prefix = dir + "/";
    const auto below = directories.lower_bound(prefix);
    if (below != directories.end() && below->compare(0, prefix.size(), prefix) == 0) {
        return true;
    }
    for (const auto& kv : files) {
        if (kv.second.exists && kv.first.compare(0, prefix.size(), prefix) == 0) {
            return true;
        }
    }
    return false;
}

InMemoryFileSystem::InMemoryDirectory::InMemoryDirectory(InMemoryFileSystem& fs, std::string path)
    : mFs(fs), mGivenPath(std::move(path)), mCanonicalPath(canonicalPath(mGivenPath))
{
}

void InMemoryFileSystem::InMemoryDirectory::setPath(const char* path)
{
    mGivenPath = path != nullptr ? path : "";
    mCanonicalPath = canonicalPath(mGivenPath);
    mOpen = false;
    mEntries.clear();
    mCursor = 0;
}

const char* InMemoryFileSystem::InMemoryDirectory::getPath() const
{
    return mGivenPath.c_str();
}

bool InMemoryFileSystem::InMemoryDirectory::exist() const
{
    return mFs.directoryExists(mCanonicalPath);
}

bool InMemoryFileSystem::InMemoryDirectory::rename(const char* newPath)
{
    (void)newPath;
    return false;
}

bool InMemoryFileSystem::InMemoryDirectory::remove()
{
    return mFs.remove(mCanonicalPath.c_str());
}

bool InMemoryFileSystem::InMemoryDirectory::create()
{
    if (isRoot(mCanonicalPath)) {
        return true;
    }
    const std::string parent = parentOf(mCanonicalPath);
    if (mFs.fileExistsAt(mCanonicalPath) || !mFs.directoryExists(parent)) {
        return false;
    }
    const uint64_t created = mFs.createdForNewDirectory(mCanonicalPath);
    if (mFs.directories.insert(mCanonicalPath).second) {
        mFs.mDirectoryCreated[mCanonicalPath] = created;
    }
    return true;
}

void InMemoryFileSystem::InMemoryDirectory::snapshot()
{
    mEntries.clear();
    mCursor = 0;

    std::map<std::string, Entry> byName;
    const auto addDirectory = [&byName](const std::string& name, uint64_t created) {
        auto [it, added] = byName.emplace(name, Entry{ name, true, 0, created });
        if (!added) {
            it->second.created = std::min(it->second.created, created);
        }
    };
    const std::string prefix = isRoot(mCanonicalPath) ? std::string() : mCanonicalPath + "/";

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
            byName[rest] = Entry{ rest, false, kv.second.content.size(), kv.second.created };
        } else {
            addDirectory(rest.substr(0, slash), kv.second.created);
        }
    }

    for (const std::string& dir : mFs.directories) {
        if (dir.compare(0, prefix.size(), prefix) != 0) {
            continue;
        }
        const std::string rest = dir.substr(prefix.size());
        const auto created = mFs.mDirectoryCreated.find(dir);
        addDirectory(rest.substr(0, rest.find('/')),
                     created == mFs.mDirectoryCreated.end() ? 0 : created->second);
    }

    mEntries.reserve(byName.size());
    for (auto& kv : byName) {
        mEntries.push_back(std::move(kv.second));
    }
    std::stable_sort(mEntries.begin(), mEntries.end(), [](const Entry& a, const Entry& b) {
        return a.created < b.created;
    });
}

bool InMemoryFileSystem::InMemoryDirectory::open()
{
    if (!mFs.directoryExists(mCanonicalPath)) {
        return false;
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
        return true;
    }
    if (mCursor >= mEntries.size()) {
        return false;
    }

    const Entry& entry = mEntries[mCursor++];
    std::strncpy(item.name, entry.name.c_str(), sizeof(item.name) - 1);
    item.name[sizeof(item.name) - 1] = '\0';
    item.isDir    = entry.isDir;
    item.isHidden = entry.name[0] == '.';
    item.isSystem = false;
    item.size     = entry.size;
    item.utc      = 0;
    return true;
}

bool InMemoryFileSystem::InMemoryDirectory::close()
{
    if (!mOpen) {
        return false;
    }
    mOpen = false;
    mEntries.clear();
    mCursor = 0;
    return true;
}

uint64_t InMemoryFileSystem::createdForNewDirectory(const std::string& canonical)
{
    if (!directoryExists(canonical)) {
        return nextCreated();
    }
    uint64_t oldest = UINT64_MAX;
    const std::string prefix = canonical + "/";
    for (const auto& kv : files) {
        if (kv.second.exists && kv.first.compare(0, prefix.size(), prefix) == 0) {
            oldest = std::min(oldest, kv.second.created);
        }
    }
    for (const auto& kv : mDirectoryCreated) {
        if (directories.count(kv.first) != 0 && kv.first.compare(0, prefix.size(), prefix) == 0) {
            oldest = std::min(oldest, kv.second);
        }
    }
    return oldest;
}

bool InMemoryFileSystem::mkdir(const char* path)
{
    if (path == nullptr) {
        return false;
    }
    const std::string requested = canonicalPath(path);
    for (std::string dir = requested; !isRoot(dir); dir = parentOf(dir)) {
        if (fileExistsAt(dir)) {
            return false;
        }
    }
    for (std::string dir = requested; !isRoot(dir); dir = parentOf(dir)) {
        const uint64_t created = createdForNewDirectory(dir);
        if (!directories.insert(dir).second) {
            break;
        }
        mDirectoryCreated[dir] = created;
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
        return {};
    }
    return std::make_unique<InMemoryDirectory>(*this, path);
}

bool InMemoryFileSystem::exist(const char* path) const
{
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    auto it = files.find(canonicalPath(path));
    if (it != files.end() && it->second.exists) {
        return true;
    }
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

    if (isRoot(target) || !directoryExists(target)) {
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
    if (oldPath == nullptr || newPath == nullptr || failRenames) {
        return false;
    }
    const std::string source = canonicalPath(oldPath);
    auto it = files.find(source);
    if (it == files.end() || !it->second.exists) {
        return false;
    }
    const std::string target = canonicalPath(newPath);
    if (!canHoldFileAt(target)) {
        return false;
    }
    if (source == target) {
        return true;
    }
    // As on the watch (f_rename), an existing target is not replaced.
    if (fileExistsAt(target)) {
        return false;
    }
    InMemoryFileEntry moved = std::move(it->second);
    files.erase(it);
    moved.created = nextCreated();
    files[target] = std::move(moved);
    return true;
}

bool InMemoryFileSystem::copy(const char* oldPath, const char* newPath)
{
    if (oldPath == nullptr || newPath == nullptr) {
        return false;
    }
    const std::string source = canonicalPath(oldPath);
    auto it = files.find(source);
    if (it == files.end() || !it->second.exists) {
        return false;
    }
    const std::string target = canonicalPath(newPath);
    if (!canHoldFileAt(target)) {
        return false;
    }
    if (source == target) {
        return true;
    }
    InMemoryFileEntry entry = it->second;
    entry.created = fileExistsAt(target) ? files[target].created : nextCreated();
    files[target] = std::move(entry);
    return true;
}

bool InMemoryFileSystem::objectInfo(const char* path, ObjectInfo& item) const
{
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    const std::string target = canonicalPath(path);

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

size_t& InMemoryFileSystem::PathCounts::operator[](const std::string& path)
{
    return mCounts[canonicalPath(path)];
}

size_t InMemoryFileSystem::PathCounts::at(const std::string& path) const
{
    return mCounts.at(canonicalPath(path));
}

size_t InMemoryFileSystem::PathCounts::count(const std::string& path) const
{
    return mCounts.count(canonicalPath(path));
}

bool InMemoryFileSystem::seedFile(const std::string& path, std::string content)
{
    const std::string target = canonicalPath(path);
    if (!canHoldFileAt(target)) {
        return false;
    }
    const uint64_t created = fileExistsAt(target) ? files[target].created : nextCreated();
    files[target] = InMemoryFileEntry{ std::move(content), true, created };
    return true;
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
