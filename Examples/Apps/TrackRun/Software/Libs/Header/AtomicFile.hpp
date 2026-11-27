/**
 ******************************************************************************
 * @file    AtomicFile.hpp
 * @brief   Replacing a small file so a crash or power loss never leaves it
 *          half written.
 *
 *          The new contents go to <path>.tmp, written and flushed in full.
 *          commit() then moves <path> aside to <path>.bak, renames the .tmp
 *          into its place and removes the .bak. Rename does not overwrite on
 *          the watch (FatFs) or in the Windows simulator, hence the move
 *          aside. At every step one whole file exists: loadPath() picks
 *          <path>, else the .tmp (the newer, whole before <path> moved),
 *          else the .bak, and renames a side file it picks to <path> so the
 *          next save cannot overwrite the only copy.
 ******************************************************************************
 */

#ifndef ATOMIC_FILE_HPP
#define ATOMIC_FILE_HPP

#include "SDK/Interfaces/IFileSystem.hpp"

#include <cstddef>

namespace AtomicFile {

/** path with suffix appended, into out. False if it does not fit. */
bool sidePath(const char* path, const char* suffix, char* out, size_t n);

/** Puts <path>.tmp, already written and closed, in place of <path>. */
bool commit(SDK::Interface::IFileSystem& fs, const char* path);

/** The file to load: path, else <path>.tmp, else <path>.bak, either renamed
 *  to path (buf holds its name if the rename fails), else nullptr. */
const char* loadPath(SDK::Interface::IFileSystem& fs, const char* path, char* buf, size_t n);

} // namespace AtomicFile

#endif // ATOMIC_FILE_HPP
