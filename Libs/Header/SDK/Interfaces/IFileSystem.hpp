/**
 ******************************************************************************
 * @file    IFileSystem.hpp
 * @date    06-12-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Interface for file/directory operations.
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include <cstdint>
#include <ctime>
#include <memory>
#include <cstdio>


namespace SDK::Interface
{

// Forward declarations
class IFile;
class IDirectory;

/**
 * @brief   Interface for the file system.
 *
 * Provides an abstraction for creating files and directories, and performing
 * operations such as removing and renaming.
 */
class IFileSystem {
public:

    ///< Maximum length of a filesystem object path including '\0'.
    static constexpr size_t skMaxPathLen = 256;

    /**
     * @brief   Structure to store directory item information.
     */
    struct ObjectInfo {
        char name[skMaxPathLen];   ///< Name of the item.
        bool isDir;       ///< 'true' if the item is a directory.
        bool isHidden;    ///< 'true' if the item is hidden.
        bool isSystem;    ///< 'true' if the item is a system file.
        size_t size;      ///< Size of the item in bytes (for files).
        time_t utc;       ///< UTC creation time
    };

    /**
     * @brief   Creates the directory (and sub directories) if it does not exist.
     * @retval  'true' if the directory was successfully created or existed,
     *          'false' otherwise.
     */
    virtual bool mkdir(const char* path) = 0;

    /**
     * @brief   Creates a file object.
     * @param   path: Path to the file.
     * @retval  Unique pointer to the created file object.
     */
    virtual std::unique_ptr<IFile> file(const char* path) = 0;

    /**
     * @brief   Creates a directory object.
     * @param   path: Path to the directory.
     * @retval  Unique pointer to the created directory object.
     */
    virtual std::unique_ptr<IDirectory> dir(const char* path) = 0;

    /**
     * @brief   Checks if a file/directory exists.
     * @param   path: Path to the file.
     * @retval  'true' if the file exists, 'false' otherwise.
     */
    virtual bool exist(const char* path) const = 0;

    /**
     * @brief   Removes a file or directory.
     * @param   path: Path to the file or directory.
     * @retval  'true' if the item was successfully removed, 'false' otherwise.
     */
    virtual bool remove(const char* path) = 0;

    /**
     * @brief   Renames a file or directory.
     * @param   oldPath: Current path of the item.
     * @param   newPath: New path for the item.
     * @retval  'true' if the item was successfully renamed, 'false' otherwise.
     */
    virtual bool rename(const char* oldPath, const char* newPath) = 0;

    /**
     * @brief   Copy a file to another location.
     * @param   oldPath: Current path of the item.
     * @param   newPath: New path for the item.
     * @retval  'true' if the item was successfully moved, 'false' otherwise.
     */
    virtual bool copy(const char* oldPath, const char* newPath) = 0;

    /**
     * @brief   Get object info.
     * @param   path: Path to the directory.
     * @param   name: Reference to save item info.
     * @retval  Execution status. 'true' - if success, 'false' - otherwise.
     */
    virtual bool objectInfo(const char* path, ObjectInfo& item) const = 0;

protected:

    /**
     * @brief   Destructor.
     */
    virtual ~IFileSystem() = default;

};


/**
 * @brief   Base interface for filesystem objects.
 *
 * Provides common functionality for both files and directories.
 */
class IFsObject {
public:

    /**
     * @brief   Destructor.
     */
    virtual ~IFsObject() = default;

    /**
     * @brief   Sets the path of the object.
     * @param   path: Path to the file or directory.
     */
    virtual void setPath(const char* path) = 0;

    /**
     * @brief   Returns the object's path.
     * @retval  Pointer to the path string.
     */
    virtual const char* getPath() const = 0;

    /**
     * @brief   Checks if the object exists.
     * @retval  'true' if the object exists, 'false' otherwise.
     */
    virtual bool exist() const = 0;

    /**
     * @brief   Rename the object.
     * @param   newPath: New object path.
     * @retval  'true' if the file was successfully renamed, 'false' otherwise.
     */
    virtual bool rename(const char* newPath) = 0;

    /**
     * @brief   Removes the object from the filesystem.
     * @retval  'true' if the object was successfully removed, 'false' otherwise.
     */
    virtual bool remove() = 0;

};

/**
 * @brief   Interface for file operations.
 *
 * Provides an abstraction over file operations such as opening, reading,
 * writing, and managing the file's state.
 */
class IFile : public IFsObject {
public:

    /**
     * @brief   Destructor.
     */
    virtual ~IFile() = default;

    /**
     * @brief   Returns the size of the file in bytes.
     * @retval  File size in bytes.
     */
    virtual size_t size() const = 0;

    /**
     * @brief   Opens the file.
     * @param   wMode: Open in write mode if true, otherwise in read mode.
     * @param   override: Override the file if it exists.
     * @retval  'true' if the file was successfully opened, 'false' otherwise.
     */
    virtual bool open(bool wMode = false, bool override = false) = 0;

    /**
     * @brief   Checks if the file is open.
     * @retval  'true' if the file is open, 'false' otherwise.
     */
    virtual bool isOpen() const = 0;

    /**
     * @brief   Closes the file.
     * @retval  'true' if the file was successfully closed, 'false' otherwise.
     */
    virtual bool close() = 0;

    /**
     * @brief   Reads data from the file.
     * @param   buff: Buffer to store the read data.
     * @param   btr: Number of bytes to read.
     * @param   br: Reference to store the number of bytes actually read.
     * @retval  'true' if the read was successful, 'false' otherwise.
     */
    virtual bool read(char* buff, size_t btr, size_t& br) = 0;

    /**
     * @brief   Writes data to the file.
     * @param   buff: Buffer containing the data to write.
     * @param   btw: Number of bytes to write.
     * @param   bw: Reference to store the number of bytes actually written.
     * @retval  'true' if the write was successful, 'false' otherwise.
     */
    virtual bool write(const char* buff, size_t btw, size_t& bw) = 0;

    /**
     * @brief   Sets the current file position.
     * @param   offset: New position in bytes.
     * @retval  'true' if the position was successfully set, 'false' otherwise.
     */
    virtual bool seek(size_t offset) = 0;

    /**
     * @brief   Truncates the file at the given offset.
     * @param   offset: Position to truncate the file.
     * @retval  'true' if the file was successfully truncated, 'false' otherwise.
     */
    virtual bool truncate(size_t offset) = 0;

    /**
     * @brief   Synchronizes the file's data to the storage medium.
     * @retval  'true' if the flush was successful, 'false' otherwise.
     */
    virtual bool flush() = 0;

    /**
     * @brief   Returns the current position in the file.
     * @retval  Current file position in bytes.
     */
    virtual size_t getPosition() const = 0;

};



/**
 * @brief   Interface for directory operations.
 *
 * Provides an abstraction over directory management, including opening,
 * reading contents, and creating directories.
 */
class IDirectory : public IFsObject {
public:

    /**
     * @brief   Destructor.
     */
    virtual ~IDirectory() = default;

    /**
     * @brief   Creates the directory if it does not exist.
     * @retval  'true' if the directory was successfully created,
     *          'false' otherwise.
     */
    virtual bool create() = 0;

    /**
     * @brief   Opens the directory for reading.
     * @retval  'true' if the directory was successfully opened,
     *          'false' otherwise.
     */
    virtual bool open() = 0;

    /**
     * @brief   Checks if the directory is open.
     * @retval  'true' if the directory is open, 'false' otherwise.
     */
    virtual bool isOpen() const = 0;

    /**
     * @brief   Reads the next item in the directory.
     * @param   item: Reference to store the directory item information.
     * @param   reset: Reset the read position if true.
     * @retval  'true' if the item was successfully read, 'false' otherwise.
     */
    virtual bool readNext(IFileSystem::ObjectInfo& item, bool reset = false) = 0;

    /**
     * @brief   Closes the directory.
     * @retval  'true' if the directory was successfully closed,
     *          'false' otherwise.
     */
    virtual bool close() = 0;

};

} // namespace SDK::Interfaces
