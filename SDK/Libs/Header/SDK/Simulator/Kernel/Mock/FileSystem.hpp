
#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <windows.h>

#include "SDK/Interfaces/IFileSystem.hpp"

namespace Mock
{

/**
 * @brief Implementation of the IFile interface for Windows.
 */
class File : public SDK::Interface::IFile {
public:

    File(const char *prefix, const char *relativePath);

    ~File() = default;


    virtual void setPath(const char *path) override;

    virtual const char *getPath() const override;

    virtual bool exist() const override;

    virtual bool rename(const char *newPath) override;

    virtual bool remove() override;

    virtual size_t size() const override;

    virtual bool open(bool wMode = false, bool override = false) override;

    virtual bool isOpen() const override;

    virtual bool close() override;

    virtual bool read(char *buff, size_t btr, size_t &br) override;

    virtual bool write(const char *buff, size_t btw, size_t &bw) override;

    virtual bool seek(size_t offset) override;

    virtual bool truncate(size_t offset) override;

    virtual bool flush() override;

    virtual size_t getPosition() const override;

private:
    std::string path;
    std::fstream file;
    bool isOpenFlag = false;
    char pathBuffer[SDK::Interface::IFileSystem::skMaxPathLen]; // Buffer to hold the path without prefix

    const char *mPathPrefix;
};


/**
 * @brief Implementation of the IDirectory interface for Windows.
 */
class Directory : public SDK::Interface::IDirectory {
public:
    Directory(const char *prefix, const char *relativePath);

    virtual ~Directory() = default;

    virtual void setPath(const char *path) override;

    virtual const char *getPath() const override;

    virtual bool exist() const override;

    virtual bool rename(const char *newPath) override;

    virtual bool remove() override;

    virtual bool create() override;

    virtual bool open() override;

    virtual bool isOpen() const override;

    virtual bool readNext(SDK::Interface::IFileSystem::ObjectInfo &item, bool reset = false) override;

    virtual bool close() override;

private:
    std::string path;
    HANDLE handle;
    WIN32_FIND_DATAA findData;
    bool isOpenFlag = false;
    char pathBuffer[SDK::Interface::IFileSystem::skMaxPathLen]; // Buffer to hold the path without prefix

    const char *mPathPrefix;
};


/**
 * @brief Implementation of the IFileSystem interface for Windows.
 */
class FileSystem : public SDK::Interface::IFileSystem {
public:

    FileSystem(const char *rootPath);

    std::string getRootPath();

    virtual bool mkdir(const char *path) override;

    virtual std::unique_ptr<SDK::Interface::IFile> file(const char *path) override;

    virtual std::unique_ptr<SDK::Interface::IDirectory> dir(const char *path) override;

    virtual bool exist(const char *path) const override;

    virtual bool remove(const char *path) override;

    virtual bool rename(const char *oldPath, const char *newPath) override;

    virtual bool copy(const char *oldPath, const char *newPath) override;

    virtual bool objectInfo(const char *path, FileSystem::ObjectInfo &item) const override;

private:
    const char *mPathPrefix;
};


} // namespace Mock