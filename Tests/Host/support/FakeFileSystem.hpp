/**
 * @file    FakeFileSystem.hpp
 * @brief   In-memory SDK::Interface::IFileSystem for host unit tests.
 *
 * Backs files with std::string and tracks created directories so persistence
 * paths (load / finalise / backup / mkdir) can be exercised without a real
 * filesystem. Only the operations the calibrator uses are meaningfully
 * implemented; the rest are minimal stubs.
 */

#ifndef __FAKE_FILE_SYSTEM_HPP
#define __FAKE_FILE_SYSTEM_HPP

#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <string>

#include "SDK/Interfaces/IFileSystem.hpp"

namespace SDK::Test
{

class FakeFileSystem : public SDK::Interface::IFileSystem {
public:
    // --- Test introspection -------------------------------------------------

    /// Inject a file that already exists on the (fake) disk.
    void seedFile(const std::string &path, const std::string &contents)
    {
        mFiles[path] = contents;
    }

    bool hasFile(const std::string &path) const
    {
        return mFiles.find(path) != mFiles.end();
    }

    std::string fileContents(const std::string &path) const
    {
        auto it = mFiles.find(path);
        return it != mFiles.end() ? it->second : std::string {};
    }

    bool dirCreated(const std::string &path) const
    {
        return mDirs.find(path) != mDirs.end();
    }

    int mkdirCalls() const { return mMkdirCalls; }

    // --- IFileSystem --------------------------------------------------------

    bool mkdir(const char *path) override
    {
        ++mMkdirCalls;
        mDirs.insert(path);
        return true;  // create-or-exists both succeed
    }

    std::unique_ptr<SDK::Interface::IFile> file(const char *path) override;

    std::unique_ptr<SDK::Interface::IDirectory> dir(const char *) override
    {
        return nullptr;
    }

    bool exist(const char *path) const override
    {
        return mFiles.find(path) != mFiles.end();
    }

    bool remove(const char *path) override
    {
        return mFiles.erase(path) > 0;
    }

    bool rename(const char *oldPath, const char *newPath) override
    {
        if (std::strcmp(oldPath, newPath) == 0) {
            return mFiles.count(oldPath) > 0;  // self-rename: no-op, don't delete
        }
        auto it = mFiles.find(oldPath);
        if (it == mFiles.end()) {
            return false;
        }
        mFiles[newPath] = it->second;
        mFiles.erase(it);
        return true;
    }

    bool copy(const char *oldPath, const char *newPath) override
    {
        auto it = mFiles.find(oldPath);
        if (it == mFiles.end()) {
            return false;
        }
        mFiles[newPath] = it->second;
        return true;
    }

    bool objectInfo(const char *path, ObjectInfo &item) const override
    {
        auto it = mFiles.find(path);
        if (it == mFiles.end()) {
            return false;
        }
        item = ObjectInfo {};
        item.size = it->second.size();
        return true;
    }

private:
    friend class FakeFile;
    std::map<std::string, std::string> mFiles;
    std::set<std::string>              mDirs;
    int                                mMkdirCalls = 0;
};

class FakeFile : public SDK::Interface::IFile {
public:
    FakeFile(FakeFileSystem &fs, const char *path) : mFs(fs), mPath(path) {}

    // --- IFsObject ----------------------------------------------------------
    void setPath(const char *path) override { mPath = path; }
    const char *getPath() const override { return mPath.c_str(); }
    bool exist() const override { return mFs.exist(mPath.c_str()); }
    bool rename(const char *newPath) override
    {
        if (!mFs.rename(mPath.c_str(), newPath)) {
            return false;
        }
        mPath = newPath;  // keep this handle pointing at the renamed entry
        return true;
    }
    bool remove() override { return mFs.remove(mPath.c_str()); }

    // --- IFile --------------------------------------------------------------
    size_t size() const override
    {
        if (mOpen) {
            return mData.size();
        }
        auto it = mFs.mFiles.find(mPath);
        return it != mFs.mFiles.end() ? it->second.size() : 0;
    }

    bool open(bool wMode = false, bool override = false) override
    {
        // Reset handle state up front so a failed open leaves a clean, closed
        // handle even if this object was previously open.
        mOpen  = false;
        mWrite = wMode;
        mPos   = 0;
        mData.clear();
        if (wMode) {
            auto it = mFs.mFiles.find(mPath);
            mData = (!override && it != mFs.mFiles.end()) ? it->second
                                                          : std::string {};
        } else {
            auto it = mFs.mFiles.find(mPath);
            if (it == mFs.mFiles.end()) {
                return false;
            }
            mData = it->second;
        }
        mOpen = true;
        return true;
    }

    bool isOpen() const override { return mOpen; }

    bool close() override
    {
        if (mOpen && mWrite) {
            mFs.mFiles[mPath] = mData;
        }
        mOpen = false;
        return true;
    }

    bool read(char *buff, size_t btr, size_t &br) override
    {
        if (!mOpen || mWrite) {
            br = 0;
            return false;
        }
        const size_t avail = mPos < mData.size() ? mData.size() - mPos : 0;
        br = btr < avail ? btr : avail;
        std::memcpy(buff, mData.data() + mPos, br);
        mPos += br;
        return true;
    }

    bool write(const char *buff, size_t btw, size_t &bw) override
    {
        if (!mOpen || !mWrite) {
            bw = 0;
            return false;
        }
        if (mPos == mData.size()) {
            mData.append(buff, btw);
        } else {
            if (mPos + btw > mData.size()) {
                mData.resize(mPos + btw);
            }
            std::memcpy(&mData[mPos], buff, btw);
        }
        mPos += btw;
        bw = btw;
        return true;
    }

    bool seek(size_t offset) override
    {
        if (!mOpen) {
            return false;  // cannot position a closed file
        }
        // Only a writable handle may extend the file. In read mode, seeking past
        // EOF is allowed but must NOT fabricate zero-filled data — a subsequent
        // read() then returns 0 bytes (read() clamps when mPos > size), matching
        // real-filesystem semantics.
        if (offset > mData.size() && mWrite) {
            mData.resize(offset);
        }
        mPos = offset;
        return true;
    }

    bool truncate(size_t offset) override
    {
        if (!mOpen || !mWrite) {
            return false;  // truncation requires an open, writable file
        }
        if (offset < mData.size()) {
            mData.resize(offset);
        }
        if (mPos > mData.size()) {
            mPos = mData.size();  // keep the cursor in bounds after shrinking
        }
        return true;
    }

    bool flush() override
    {
        if (mOpen && mWrite) {
            mFs.mFiles[mPath] = mData;
        }
        return true;
    }

    size_t getPosition() const override { return mPos; }

private:
    FakeFileSystem &mFs;
    std::string     mPath;
    std::string     mData;
    size_t          mPos   = 0;
    bool            mOpen  = false;
    bool            mWrite = false;
};

inline std::unique_ptr<SDK::Interface::IFile>
FakeFileSystem::file(const char *path)
{
    return std::unique_ptr<SDK::Interface::IFile>(new FakeFile(*this, path));
}

} // namespace SDK::Test

#endif /* __FAKE_FILE_SYSTEM_HPP */
