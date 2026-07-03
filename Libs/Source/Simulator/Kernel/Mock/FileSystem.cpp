/**
 ******************************************************************************
 * @file    FileSystem.cpp
 * @date    04-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Mock for IFileSystem interface (Windows + POSIX).
 ******************************************************************************
 *
 * The POSIX path (the #else branch) is implemented on file descriptors and
 * dirent rather than std::fstream. The IFile contract exposes a single
 * read/write position, which maps exactly onto an fd's offset; std::fstream
 * keeps separate get/put positions and its append mode ignores seek(), so it
 * cannot honour a seek() + write() after a non-truncating open. The Windows
 * path retains the original std::fstream implementation, so some behaviour
 * differs between the two backends (see File::open and Directory::readNext).
 *
 ******************************************************************************
 */

#include "SDK/Simulator/Kernel/Mock/FileSystem.hpp"

#define LOG_MODULE_PRX      "Mock.FileSystem"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Wrappers/StdLibWrappers.h"

#include <cstring>
#include <filesystem>
#include <sys/stat.h>

#ifdef _WIN32
#include <vector>
#else
#include <cerrno>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace SDK::Simulator::Mock
{

// Method to add prefix to the path
static std::string AddPrefix(const char *prefix, const char *path)
{
    return std::string(prefix) + path;
}

#ifdef _WIN32
// Function to convert FILETIME to time_t (UTC)
static uint32_t FileTimeToUnixTime(const FILETIME &fileTime)
{
    // Constant to convert from January 1, 1601 to January 1, 1970 in 100-nanosecond intervals
    const uint64_t EPOCH_DIFFERENCE = 116444736000000000ULL;

    // Combine the two parts of FILETIME into a 64-bit value
    uint64_t time = (static_cast<uint64_t>(fileTime.dwHighDateTime) << 32) | fileTime.dwLowDateTime;

    // Convert to seconds by subtracting the difference between epochs
    return static_cast<uint32_t>((time - EPOCH_DIFFERENCE) / 10000000ULL);
}
#endif



File::File(const char *prefix, const char *relativePath)
    : mPathPrefix(prefix)
{
    setPath(relativePath);
}

File::~File()
{
    close(); // release the handle on destruction (RAII); close() is idempotent
}

void File::setPath(const char *path)
{
    this->path = AddPrefix(mPathPrefix, path);

    // Store the caller-facing (prefix-less) path; safe_strcpy always NUL-terminates.
    safe_strcpy(pathBuffer, path, sizeof(pathBuffer));
}

const char *File::getPath() const
{
    return pathBuffer; // Return the path stored in the buffer
}

bool File::exist() const
{
#ifdef _WIN32
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
#else
    struct stat st;
    return ::stat(path.c_str(), &st) == 0;
#endif
}

bool File::rename(const char *newPath)
{
    return std::rename(path.c_str(), AddPrefix(mPathPrefix, newPath).c_str()) == 0;
}

bool File::remove()
{
    return std::remove(path.c_str()) == 0;
}

size_t File::size() const
{
    struct stat st;
    if (::stat(path.c_str(), &st) == 0) {
        return static_cast<size_t>(st.st_size);
    }
    return 0;
}

bool File::open(bool wMode, bool override)
{
#ifdef _WIN32
    std::ios_base::openmode mode = std::ios::in | std::ios::binary;
    if (wMode) {
        mode = std::ios::out | std::ios::binary | (override ? std::ios::trunc : std::ios::app);
    }
    file.open(path, mode);
    isOpenFlag = file.is_open();
    return isOpenFlag;
#else
    // Read      -> read-only.
    // Write+override -> create/truncate, read+write, position at start.
    // Write     -> open-or-create WITHOUT truncating, read+write, position at
    //              start. Deliberately not append: append would ignore lseek and
    //              break seek()+write() (e.g. patching a header/CRC after a
    //              streamed write).
    int flags;
    if (!wMode) {
        flags = O_RDONLY;
    } else if (override) {
        flags = O_RDWR | O_CREAT | O_TRUNC;
    } else {
        flags = O_RDWR | O_CREAT;
    }

    // Release any descriptor from a prior open() so reopening cannot leak it.
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }

    fd = ::open(path.c_str(), flags, 0644);
    return fd >= 0;
#endif
}

bool File::isOpen() const
{
#ifdef _WIN32
    return isOpenFlag;
#else
    return fd >= 0;
#endif
}

bool File::close()
{
#ifdef _WIN32
    if (isOpenFlag) {
        file.close();
        isOpenFlag = false;
        return true;
    }
    return false;
#else
    if (fd >= 0) {
        bool ok = (::close(fd) == 0);
        fd = -1;
        return ok;
    }
    return false;
#endif
}

bool File::read(char *buff, size_t btr, size_t &br)
{
    br = 0;
#ifdef _WIN32
    if (!isOpenFlag) return false;

    file.read(buff, btr);
    br = static_cast<size_t>(file.gcount());

    // Check whether the end of the file has been reached or another error has occurred
    if (file.eof()) {
        return true;  // Read completed successfully
    }
    if (file.fail() || file.bad()) {
        return false;  // An error occurred
    }
    return true;
#else
    if (fd < 0) return false;

    // Loop over short reads; a partial read at EOF is success (br < btr).
    while (br < btr) {
        ssize_t n = ::read(fd, buff + br, btr - br);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) break; // EOF
        br += static_cast<size_t>(n);
    }
    return true;
#endif
}

bool File::write(const char *buff, size_t btw, size_t &bw)
{
    bw = 0;
#ifdef _WIN32
    if (!isOpenFlag) return false;

    // Perform recording
    file.write(static_cast<const char *>(buff), btw);

    // Check if the recording was successful
    if (file.fail() || file.bad()) {
        bw = 0;  // On error, no bytes are written
        return false;
    }

    // If everything is fine, set the number of written bytes
    bw = btw;
    return true;
#else
    if (fd < 0) return false;

    // Loop over short writes so a full buffer is committed (or we report failure).
    while (bw < btw) {
        ssize_t n = ::write(fd, buff + bw, btw - bw);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) {
            return false; // no progress: cannot complete the write
        }
        bw += static_cast<size_t>(n);
    }
    return true;
#endif
}

bool File::seek(size_t offset)
{
#ifdef _WIN32
    if (!isOpenFlag) return false;
    file.seekg(offset);
    file.seekp(offset);
    return !file.fail();
#else
    if (fd < 0) return false;
    return ::lseek(fd, static_cast<off_t>(offset), SEEK_SET) != static_cast<off_t>(-1);
#endif
}

bool File::truncate(size_t offset)
{
#ifdef _WIN32
    if (!isOpenFlag) return false;

    // Close the file before trimming
    file.close();

    // Open the file in read mode
    std::ifstream inFile(path, std::ios::binary);
    if (!inFile.is_open()) return false;

    // Read data up to the specified offset
    std::vector<char> buffer(offset);
    inFile.read(buffer.data(), offset);
    inFile.close();

    // Open the file in writing mode with cleaning
    std::ofstream outFile(path, std::ios::binary | std::ios::trunc);
    if (!outFile.is_open()) return false;

    // Write the content to the specified offset
    outFile.write(buffer.data(), inFile.gcount());
    outFile.close();

    // We reopen the main thread for further work
    file.open(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) return false;

    isOpenFlag = true; // Set the open file flag
    return true;
#else
    if (fd < 0) return false;
    // ftruncate sets the exact file length; the fd offset is unchanged.
    return ::ftruncate(fd, static_cast<off_t>(offset)) == 0;
#endif
}

bool File::flush()
{
#ifdef _WIN32
    if (!isOpenFlag) return false;
    file.flush();
    return !file.fail();
#else
    if (fd < 0) return false;
    return ::fsync(fd) == 0;
#endif
}

size_t File::getPosition() const
{
#ifdef _WIN32
    if (!isOpenFlag) return 0;

    // Since tellg() cannot be called from const, use const_cast
    return static_cast<size_t>(const_cast<std::fstream &>(file).tellg());
#else
    if (fd < 0) return 0;
    off_t pos = ::lseek(fd, 0, SEEK_CUR);
    return pos < 0 ? 0 : static_cast<size_t>(pos);
#endif
}



Directory::Directory(const char *prefix, const char *relativePath)
    : mPathPrefix(prefix)
{
    setPath(relativePath);
}

Directory::~Directory()
{
    close(); // release the handle on destruction (RAII); close() is idempotent
}

void Directory::setPath(const char *path)
{
    this->path = AddPrefix(mPathPrefix, path);
    // Store the caller-facing (prefix-less) path; safe_strcpy always NUL-terminates.
    safe_strcpy(pathBuffer, path, sizeof(pathBuffer));
}

const char *Directory::getPath() const
{
    return pathBuffer; // Return the path stored in the buffer
}

bool Directory::exist() const
{
#ifdef _WIN32
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
#else
    struct stat st;
    return ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
#endif
}

bool Directory::rename(const char *newPath)
{
    return std::rename(path.c_str(), AddPrefix(mPathPrefix, newPath).c_str()) == 0;
}

bool Directory::remove()
{
    return std::remove(path.c_str()) == 0;
}

bool Directory::create()
{
#ifdef _WIN32
    return CreateDirectoryA(path.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
#else
    if (::mkdir(path.c_str(), 0755) == 0) {
        return true;
    }
    if (errno == EEXIST) {
        // Only report success if the existing object really is a directory.
        struct stat st;
        return ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
    }
    return false;
#endif
}

bool Directory::open()
{
#ifdef _WIN32
    std::string searchPath = path + "/*";
    handle = FindFirstFileA(searchPath.c_str(), &findData);
    isOpenFlag = (handle != INVALID_HANDLE_VALUE);
    hasEntry = isOpenFlag;
#else
    // Release any stream from a prior open() so reopening cannot leak it.
    if (handle != nullptr) {
        ::closedir(handle);
    }
    handle = ::opendir(path.c_str());
    isOpenFlag = (handle != nullptr);
#endif
    return isOpenFlag;
}

bool Directory::isOpen() const
{
    return isOpenFlag;
}

bool Directory::readNext(SDK::Interface::IFileSystem::ObjectInfo &item, bool reset)
{
    if (!isOpenFlag) return false;

#ifdef _WIN32
    if (reset) {
        FindClose(handle);
        return open();
    }

    if (!hasEntry) return false;

    safe_strcpy(item.name, findData.cFileName, sizeof(item.name));
    item.isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    item.isHidden = (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
    item.isSystem = (findData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0;
    item.size = findData.nFileSizeLow;
    item.utc = FileTimeToUnixTime(findData.ftCreationTime);

    hasEntry = (FindNextFileA(handle, &findData) != 0);

    return true;
#else
    if (reset) {
        ::rewinddir(handle);
        return true; // rewind only; a reset call does not read an entry (per contract)
    }

    // Skip the "." and ".." pseudo-entries so callers iterate real children only.
    struct dirent *ent = nullptr;
    do {
        ent = ::readdir(handle);
    } while (ent != nullptr &&
             (std::strcmp(ent->d_name, ".") == 0 || std::strcmp(ent->d_name, "..") == 0));

    if (ent == nullptr) {
        return false; // directory exhausted
    }

    // Populate every field; never leave one uninitialised.
    safe_strcpy(item.name, ent->d_name, sizeof(item.name));
    item.isHidden = (ent->d_name[0] == '.');
    item.isSystem = false;

    struct stat st;
    const std::string full = path + "/" + ent->d_name;
    if (::stat(full.c_str(), &st) == 0) {
        // st_mode is authoritative for isDir (d_type may be DT_UNKNOWN on some
        // filesystems) and yields a 64-bit size and a timestamp.
        item.isDir = S_ISDIR(st.st_mode);
        item.size  = static_cast<size_t>(st.st_size);
        // POSIX has no portable creation time; st_ctime (inode change time) is
        // the closest widely-available approximation. Keep the full time_t width.
        item.utc   = st.st_ctime;
    } else {
        // stat failed (e.g. dangling symlink): fall back without inventing data.
        item.isDir = (ent->d_type == DT_DIR);
        item.size  = 0;
        item.utc   = 0;
    }

    return true;
#endif
}

bool Directory::close()
{
    if (isOpenFlag) {
#ifdef _WIN32
        FindClose(handle);
        hasEntry = false;
#else
        ::closedir(handle);
        handle = nullptr;
#endif
        isOpenFlag = false;
        return true;
    }
    return false;
}



FileSystem::FileSystem(const char *rootPath)
{
    mPathPrefix = rootPath;
#ifdef _WIN32
    CreateDirectoryA(mPathPrefix, nullptr);
#else
    ::mkdir(mPathPrefix, 0755);
#endif
}

std::string FileSystem::getRootPath()
{
    std::filesystem::path relPath{ mPathPrefix };
    return std::filesystem::absolute(relPath).string();
}

bool FileSystem::mkdir(const char *path)
{
    std::string directory(path);
    std::stringstream ss(directory);
    std::string token;
    std::string currentPath;

    while (std::getline(ss, token, '/')) {
        if (token.empty()) {
            continue; // tolerate leading, trailing, and duplicate '/'
        }
        if (!currentPath.empty()) {
            currentPath += "/";
        }
        currentPath += token;

        const std::string full = AddPrefix(mPathPrefix, currentPath.c_str());
#ifdef _WIN32
        if (!CreateDirectoryA(full.c_str(), nullptr)) {
            if (GetLastError() != ERROR_ALREADY_EXISTS) {
                return false;
            }
        }
#else
        if (::mkdir(full.c_str(), 0755) != 0 && errno != EEXIST) {
            return false;
        }
#endif
    }

    return true;
}

std::unique_ptr<SDK::Interface::IFile> FileSystem::file(const char *path)
{
    return std::make_unique<File>(mPathPrefix, path);
}

std::unique_ptr<SDK::Interface::IDirectory> FileSystem::dir(const char *path)
{
    return std::make_unique<Directory>(mPathPrefix, path);
}

bool  FileSystem::exist(const char *path) const
{
#ifdef _WIN32
    return GetFileAttributesA(AddPrefix(mPathPrefix, path).c_str()) != INVALID_FILE_ATTRIBUTES;
#else
    struct stat st;
    return ::stat(AddPrefix(mPathPrefix, path).c_str(), &st) == 0;
#endif
}

bool  FileSystem::remove(const char *path)
{
    return std::remove(AddPrefix(mPathPrefix, path).c_str()) == 0;
}

bool  FileSystem::rename(const char *oldPath, const char *newPath)
{
    return std::rename(AddPrefix(mPathPrefix, oldPath).c_str(), AddPrefix(mPathPrefix, newPath).c_str()) == 0;
}

bool  FileSystem::copy(const char *oldPath, const char *newPath)
{
    std::ifstream source(AddPrefix(mPathPrefix, oldPath).c_str(), std::ios::binary);
    std::ofstream destination(AddPrefix(mPathPrefix, newPath).c_str(), std::ios::binary);

    if (!source || !destination) {
        return false;
    }

    destination << source.rdbuf();

    // An empty source sets failbit on the rdbuf insertion, which is not an error;
    // only a real I/O failure sets badbit.
    return !destination.bad() && !source.bad();
}


bool FileSystem::objectInfo(const char *path, FileSystem::ObjectInfo &item) const
{
#ifdef _WIN32
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(AddPrefix(mPathPrefix, path).c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return false;
    safe_strcpy(item.name, findData.cFileName, sizeof(item.name));
    item.isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    item.isHidden = (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
    item.isSystem = (findData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0;
    item.size = findData.nFileSizeLow;
    item.utc = FileTimeToUnixTime(findData.ftCreationTime);
    FindClose(hFind);
    return true;
#else
    std::string full = AddPrefix(mPathPrefix, path);
    struct stat st;
    if (::stat(full.c_str(), &st) != 0) {
        return false;
    }

    // Derive the leaf name from the path; strip trailing slashes first so a
    // path like "a/b/" yields "b" rather than an empty name.
    while (full.size() > 1 && full.back() == '/') {
        full.pop_back();
    }
    const size_t pos = full.rfind('/');
    const std::string name = (pos != std::string::npos) ? full.substr(pos + 1) : full;

    safe_strcpy(item.name, name.c_str(), sizeof(item.name));
    item.isDir = S_ISDIR(st.st_mode);
    item.isHidden = (!name.empty() && name[0] == '.');
    item.isSystem = false;
    item.size = static_cast<size_t>(st.st_size);
    item.utc = st.st_ctime; // time_t; keep full width (no uint32 truncation)
    return true;
#endif
}

} /* namespace Simulator */
