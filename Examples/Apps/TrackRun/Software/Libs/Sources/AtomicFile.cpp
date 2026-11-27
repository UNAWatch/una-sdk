/**
 ******************************************************************************
 * @file    AtomicFile.cpp
 * @brief   Replacing a small file so it is never half written (AtomicFile.hpp).
 ******************************************************************************
 */

#include "AtomicFile.hpp"

#include <cstdio>
#include <initializer_list>

namespace AtomicFile {

bool sidePath(const char* path, const char* suffix, char* out, size_t n)
{
    const int len = std::snprintf(out, n, "%s%s", path, suffix);
    return len > 0 && static_cast<size_t>(len) < n;
}

bool commit(SDK::Interface::IFileSystem& fs, const char* path)
{
    char tmp[SDK::Interface::IFileSystem::skMaxPathLen];
    char bak[SDK::Interface::IFileSystem::skMaxPathLen];
    if (!sidePath(path, ".tmp", tmp, sizeof tmp) || !sidePath(path, ".bak", bak, sizeof bak)) {
        return false;
    }
    const bool hadOld = fs.exist(path);
    if (hadOld) {
        if (fs.exist(bak)) {
            fs.remove(bak);
        }
        if (!fs.rename(path, bak)) {
            return false; // the old file is untouched
        }
    }
    if (!fs.rename(tmp, path)) {
        if (hadOld) {
            fs.rename(bak, path); // the old one back
        }
        return false;
    }
    if (hadOld) {
        fs.remove(bak);
    }
    return true;
}

const char* loadPath(SDK::Interface::IFileSystem& fs, const char* path, char* buf, size_t n)
{
    if (fs.exist(path)) {
        return path;
    }
    // A save was interrupted between its renames: the side file is put in
    // place now, so the next save (which rewrites the .tmp) cannot lose it.
    for (const char* suffix : {".tmp", ".bak"}) {
        if (sidePath(path, suffix, buf, n) && fs.exist(buf)) {
            return fs.rename(buf, path) ? path : buf;
        }
    }
    return nullptr;
}

} // namespace AtomicFile
