#include "KernelTestDoubles.hpp"

#include <algorithm>
#include <cstring>
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
    , mPath(std::move(path))
{
}

void InMemoryFileSystem::InMemoryFile::setPath(const char* path)
{
    mPath = path != nullptr ? path : "";
}

const char* InMemoryFileSystem::InMemoryFile::getPath() const
{
    return mPath.c_str();
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
    mPath = newPath;
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

InMemoryFileSystem::EmptyDirectory::EmptyDirectory(std::string path)
    : mPath(std::move(path))
{
}

void InMemoryFileSystem::EmptyDirectory::setPath(const char* path)
{
    mPath = path != nullptr ? path : "";
}

const char* InMemoryFileSystem::EmptyDirectory::getPath() const
{
    return mPath.c_str();
}

bool InMemoryFileSystem::EmptyDirectory::exist() const
{
    return true;
}

bool InMemoryFileSystem::EmptyDirectory::rename(const char* newPath)
{
    if (newPath == nullptr) {
        return false;
    }
    mPath = newPath;
    return true;
}

bool InMemoryFileSystem::EmptyDirectory::remove()
{
    return true;
}

bool InMemoryFileSystem::EmptyDirectory::create()
{
    return true;
}

bool InMemoryFileSystem::EmptyDirectory::open()
{
    mOpen = true;
    return true;
}

bool InMemoryFileSystem::EmptyDirectory::isOpen() const
{
    return mOpen;
}

bool InMemoryFileSystem::EmptyDirectory::readNext(SDK::Interface::IFileSystem::ObjectInfo& item, bool reset)
{
    (void)item;
    (void)reset;
    return false;
}

bool InMemoryFileSystem::EmptyDirectory::close()
{
    mOpen = false;
    return true;
}

bool InMemoryFileSystem::mkdir(const char* path)
{
    (void)path;
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
    return std::make_unique<EmptyDirectory>(path != nullptr ? path : "");
}

bool InMemoryFileSystem::exist(const char* path) const
{
    if (path == nullptr) {
        return false;
    }
    auto it = files.find(path);
    return it != files.end() && it->second.exists;
}

bool InMemoryFileSystem::remove(const char* path)
{
    if (path == nullptr) {
        return false;
    }
    auto it = files.find(path);
    if (it == files.end()) {
        return false;
    }
    it->second.exists = false;
    it->second.content.clear();
    return true;
}

bool InMemoryFileSystem::rename(const char* oldPath, const char* newPath)
{
    if (oldPath == nullptr || newPath == nullptr) {
        return false;
    }
    auto it = files.find(oldPath);
    if (it == files.end()) {
        return false;
    }
    files[newPath] = std::move(it->second);
    files.erase(it);
    return true;
}

bool InMemoryFileSystem::copy(const char* oldPath, const char* newPath)
{
    if (oldPath == nullptr || newPath == nullptr) {
        return false;
    }
    auto it = files.find(oldPath);
    if (it == files.end()) {
        return false;
    }
    files[newPath] = it->second;
    return true;
}

bool InMemoryFileSystem::objectInfo(const char* path, ObjectInfo& item) const
{
    if (path == nullptr) {
        return false;
    }
    auto it = files.find(path);
    if (it == files.end() || !it->second.exists) {
        return false;
    }
    std::strncpy(item.name, path, sizeof(item.name) - 1);
    item.name[sizeof(item.name) - 1] = '\0';
    item.isDir = false;
    item.isHidden = false;
    item.isSystem = false;
    item.size = it->second.content.size();
    item.utc = 0;
    return true;
}

void InMemoryFileSystem::seedFile(const std::string& path, std::string content)
{
    files[path] = InMemoryFileEntry{ std::move(content), true };
}

std::string InMemoryFileSystem::readFile(const std::string& path) const
{
    auto it = files.find(path);
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
