/**
 ******************************************************************************
 * @file    FileSystem.cpp
 * @date    04-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Mock for IFileSystem interface.
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "Simulator/Kernel/Mock/FileSystem.hpp"
#include "Wrappers/StdLibWrappers.h"

#include <vector>
#include < filesystem >

namespace Mock
{

// Method to add prefix to the path
static std::string AddPrefix(const char *prefix, const char *path)
{
    return std::string(prefix) + path;
}

// Method to remove prefix from the path
static std::string RemovePrefix(const char *prefix, std::string path)
{
    return path.substr(std::strlen(prefix));
}

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



File::File(const char *prefix, const char *relativePath)
    : mPathPrefix(prefix)
{
    setPath(relativePath);
}

void File::setPath(const char *path)
{
    this->path = AddPrefix(mPathPrefix, path);
    
    // Update pathBuffer whenever the path is set
    safe_strcpy(pathBuffer, path, sizeof(pathBuffer) - 1);
}


const char *File::getPath() const
{
    return pathBuffer; // Return the path stored in the buffer
}

bool File::exist() const
{
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
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
    if (stat(path.c_str(), &st) == 0) {
        return st.st_size;
    }
    return 0;
}

bool File::open(bool wMode, bool override)
{
    std::ios_base::openmode mode = std::ios::in | std::ios::binary;
    if (wMode) {
        mode = std::ios::out | std::ios::binary | (override ? std::ios::trunc : std::ios::app);
    }
    file.open(path, mode);
    isOpenFlag = file.is_open();
    return isOpenFlag;
}

bool File::isOpen() const
{
    return isOpenFlag;
}

bool File::close()
{
    if (isOpenFlag) {
        file.close();
        isOpenFlag = false;
        return true;
    }
    return false;
}

bool File::read(char *buff, size_t btr, size_t &br)
{
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
}

bool File::write(const char *buff, size_t btw, size_t &bw)
{
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
}

bool File::seek(size_t offset)
{
    if (!isOpenFlag) return false;
    file.seekg(offset);
    return !file.fail();
}

bool File::truncate(size_t offset)
{
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
}

bool File::flush()
{
    if (!isOpenFlag) return false;
    file.flush();
    return !file.fail();
}

size_t File::getPosition() const
{
    if (!isOpenFlag) return 0;

    // Since tellg() cannot be called from const, use const_cast
    return static_cast<size_t>(const_cast<std::fstream &>(file).tellg());
}



Directory::Directory(const char *prefix, const char *relativePath) 
    : mPathPrefix(prefix)
{
    setPath(relativePath);
}

void Directory::setPath(const char *path)
{
    this->path = AddPrefix(mPathPrefix, path);
    // Update pathBuffer whenever the path is set
    safe_strcpy(pathBuffer, path, sizeof(pathBuffer) - 1);
    pathBuffer[sizeof(pathBuffer) - 1] = '\0'; // Ensure null-termination
}

const char *Directory::getPath() const
{
    return pathBuffer; // Return the path stored in the buffer
}

bool Directory::exist() const
{
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
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
    return CreateDirectoryA(path.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
}

bool Directory::open()
{
    std::string searchPath = path + "/*";
    handle = FindFirstFileA(searchPath.c_str(), &findData);
    isOpenFlag = (handle != INVALID_HANDLE_VALUE);
    return isOpenFlag;
}

bool Directory::isOpen() const
{
    return isOpenFlag;
}

bool Directory::readNext(sdk::api::FileSystem::ObjectInfo &item, bool reset)
{
    if (!isOpenFlag) return false;

    if (reset) {
        FindClose(handle);
        open();
    }

    do {
        safe_strcpy(item.name, findData.cFileName, sizeof(item.name));
        item.isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        item.isHidden = (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
        item.isSystem = (findData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0;
        item.size = findData.nFileSizeLow;
        item.utc = FileTimeToUnixTime(findData.ftCreationTime);
    } while (FindNextFileA(handle, &findData));

    return true;
}

bool Directory::close()
{
    if (isOpenFlag) {
        FindClose(handle);
        isOpenFlag = false;
        return true;
    }
    return false;
}



FileSystem::FileSystem(const char *rootPath)
{
    mPathPrefix = rootPath;
    CreateDirectoryA(mPathPrefix, nullptr);
}

std::string FileSystem::getRootPath()
{
    std::filesystem::path relPath{ mPathPrefix };
    return std::filesystem::absolute(relPath).string();
}

bool FileSystem::mkdir(const char *path)
{
    std::string directory(path);
    std::vector<std::string> directories;
    std::stringstream ss(directory);
    std::string token;

    while (std::getline(ss, token, '/')) {
        directories.push_back(token);
    }

    std::string currentPath;
    for (const auto &dir : directories) {
        if (!currentPath.empty()) {
            currentPath += "/";
        }
        currentPath += dir;
        if (!CreateDirectoryA(AddPrefix(mPathPrefix, currentPath.c_str()).c_str(), nullptr)) {
            if (GetLastError() != ERROR_ALREADY_EXISTS) {
                return false;
            }
        }
    }

    return true;
}

std::unique_ptr<sdk::api::File> FileSystem::file(const char *path)
{
    return std::make_unique<File>(mPathPrefix, path);
}

std::unique_ptr<sdk::api::Directory> FileSystem::dir(const char *path)
{
    return std::make_unique<Directory>(mPathPrefix, path);
}

bool  FileSystem::exist(const char *path) const
{
    return GetFileAttributesA(AddPrefix(mPathPrefix, path).c_str()) != INVALID_FILE_ATTRIBUTES;
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

    return true;
}


bool FileSystem::objectInfo(const char *path, FileSystem::ObjectInfo &item) const
{
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(AddPrefix(mPathPrefix, path).c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return false;
    safe_strcpy(item.name, findData.cFileName, sizeof(item.name));
    item.isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    item.size = findData.nFileSizeLow;
    item.utc = FileTimeToUnixTime(findData.ftCreationTime);
    FindClose(hFind);
    return true;
}

} /* namespace Simulator */
