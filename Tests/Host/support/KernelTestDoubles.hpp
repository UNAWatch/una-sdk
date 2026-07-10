#pragma once

#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <memory>
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
        std::string mPath;
        bool mOpen = false;
        bool mWriteMode = false;
        size_t mPos = 0;
    };

    class EmptyDirectory : public SDK::Interface::IDirectory {
    public:
        explicit EmptyDirectory(std::string path);
        ~EmptyDirectory() override = default;

        void setPath(const char* path) override;
        const char* getPath() const override;
        bool exist() const override;
        bool rename(const char* newPath) override;
        bool remove() override;

        bool create() override;
        bool open() override;
        bool isOpen() const override;
        bool readNext(SDK::Interface::IFileSystem::ObjectInfo& item, bool reset = false) override;
        bool close() override;

    private:
        std::string mPath;
        bool mOpen = false;
    };

    bool mkdir(const char* path) override;
    std::unique_ptr<SDK::Interface::IFile> file(const char* path) override;
    std::unique_ptr<SDK::Interface::IDirectory> dir(const char* path) override;
    bool exist(const char* path) const override;
    bool remove(const char* path) override;
    bool rename(const char* oldPath, const char* newPath) override;
    bool copy(const char* oldPath, const char* newPath) override;
    bool objectInfo(const char* path, ObjectInfo& item) const override;

    void seedFile(const std::string& path, std::string content);
    std::string readFile(const std::string& path) const;

    std::unordered_map<std::string, InMemoryFileEntry> files;

    // -- Test instrumentation -------------------------------------------------
    /// Number of successful flush() calls, keyed by file path.
    std::unordered_map<std::string, size_t> flushCounts;
    /// Total bytes written across all files (successful writes only).
    size_t bytesWritten = 0;
    /// Once bytesWritten reaches this threshold, subsequent write() calls fail
    /// (simulates a storage write error). Defaults to "never fail".
    size_t failWritesAfterBytes = static_cast<size_t>(-1);
    /// A write-mode open() targeting a path ending with this suffix fails without
    /// creating/touching the entry (simulates a storage error scoped to one file
    /// kind, e.g. the auxiliary ".json" summary). Empty = never fail.
    std::string failWriteOpenSuffix;
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
