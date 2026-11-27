/**
 ******************************************************************************
 * @file    TrackStoreFile.cpp
 * @brief   The stored tracks file, loaded and saved atomically.
 ******************************************************************************
 */

#include "TrackStoreFile.hpp"

#include <memory>

#define LOG_MODULE_PRX      "TrackStoreFile"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

bool TrackStoreFile::readAll(const char* path, size_t& len)
{
    len = 0;
    if (!mKernel.fs.exist(path)) {
        return false;
    }
    std::unique_ptr<SDK::Interface::IFile> f = mKernel.fs.file(path);
    if (!f || !f->open(false, false)) {
        return false;
    }
    // A store is at most kMaxSerialisedBytes; anything beyond is not read
    // (TrackStore::load counts a cut-off last line).
    size_t got = 0;
    const bool ok = f->read(mBuf, sizeof(mBuf), got);
    f->close();
    len = got;
    return ok || got > 0;
}

Oval::TrackStore::LoadReport TrackStoreFile::load(Oval::TrackStore& store, const char** fromPath)
{
    mRecovered = nullptr;
    // save() writes .tmp in full before it moves tracks.txt aside, so with
    // tracks.txt missing .tmp is the newer store and .bak the older. A file
    // whose header or lines are damaged is passed over for a whole one; a
    // whole file with no track (the last one forgotten) is taken as it is.
    // With none whole, the one that loads the most tracks.
    static const char* const kTry[] = {kPath, kTmpPath, kBakPath};
    const char* from  = nullptr;
    size_t      len   = 0;
    int         which = -1; // 0: tracks.txt, else a fallback
    int         best  = -1; // with none whole: the most tracks, the first on a tie
    uint32_t    bestN = 0;
    Oval::TrackStore::LoadReport rep;
    for (int i = 0; i < 3; ++i) {
        if (!readAll(kTry[i], len) || len == 0) {
            continue;
        }
        rep = store.load(mBuf, len);
        if (best < 0 || rep.loaded > bestN) {
            best  = i;
            bestN = static_cast<uint32_t>(rep.loaded);
        }
        if (rep.newerVersion || (rep.headerOk && rep.dropped() == 0 && rep.loaded == rep.expected)) {
            from  = kTry[i];
            which = i;
            break;
        }
    }
    if (from == nullptr && best >= 0 && readAll(kTry[best], len)) {
        from  = kTry[best];
        which = best;
        rep   = store.load(mBuf, len);
    } else if (from == nullptr) {
        rep = store.load(mBuf, 0);
    }
    if (fromPath != nullptr) {
        *fromPath = from;
    }
    mNewer = rep.newerVersion;
    if (from == nullptr) {
        LOG_INFO("No stored tracks file: no tracks yet\n");
    } else {
        LOG_INFO("Stored tracks [%s]: %u bytes, header %u, version %u%s, %u loaded of %u expected, %u lines\n",
                 from, static_cast<unsigned>(len), rep.headerOk ? 1u : 0u, rep.version,
                 rep.newerVersion ? " (newer: will not be saved over)" : "", rep.loaded, rep.expected,
                 rep.lines);
        if (rep.dropped() > 0 || rep.otherLines > 0) {
            LOG_WARNING("Stored tracks dropped: %u cut off, %u bad CRC, %u bad field, %u duplicate; %u other lines\n",
                        rep.truncated, rep.badChecksum, rep.badField, rep.duplicate, rep.otherLines);
        }
        if (which > 0) {
            LOG_WARNING("Stored tracks recovered from [%s] (a save was interrupted)\n", from);
            // Put in place, so the next save (which rewrites tracks.tmp and
            // clears tracks.bak) cannot lose the only good copy.
            mRecovered = from;
            promoteRecovered();
        }
    }
    return rep;
}

size_t TrackStoreFile::serialise(const Oval::TrackStore& store, const char*& buf)
{
    buf = mBuf;
    return store.serialise(mBuf, sizeof(mBuf));
}

bool TrackStoreFile::promoteRecovered()
{
    if (mRecovered == nullptr) {
        return true;
    }
    // A main file there failed the checks the recovered copy passed: it
    // gives way (rename never overwrites).
    if (mKernel.fs.exist(kPath) && !mKernel.fs.remove(kPath)) {
        LOG_ERROR("Stored tracks: cannot remove the damaged [%s]\n", kPath);
        return false;
    }
    if (!mKernel.fs.rename(mRecovered, kPath)) {
        LOG_ERROR("Stored tracks: cannot move [%s] to [%s]\n", mRecovered, kPath);
        return false;
    }
    mRecovered = nullptr;
    return true;
}

TrackStoreFile::SaveResult TrackStoreFile::save(const Oval::TrackStore& store)
{
    if (mNewer) {
        LOG_WARNING("Stored tracks not saved: the file was written by a newer version\n");
        return SaveResult::SkippedNewer;
    }
    if (!promoteRecovered()) {
        // The only good copy is still a side file the steps below rewrite.
        LOG_ERROR("Stored tracks not saved: the recovered copy is not in place\n");
        return SaveResult::Failed;
    }
    const size_t len = store.serialise(mBuf, sizeof(mBuf));
    if (len == 0) {
        LOG_ERROR("Stored tracks: serialise failed\n");
        return SaveResult::Failed;
    }

    // 1. The new store in full, beside the old one.
    {
        std::unique_ptr<SDK::Interface::IFile> f = mKernel.fs.file(kTmpPath);
        if (!f || !f->open(true, true)) {
            LOG_ERROR("Stored tracks: cannot open [%s]\n", kTmpPath);
            return SaveResult::Failed;
        }
        size_t wrote = 0;
        const bool ok = f->write(mBuf, len, wrote) && wrote == len && f->flush();
        f->close();
        if (!ok) {
            LOG_ERROR("Stored tracks: write to [%s] failed\n", kTmpPath);
            mKernel.fs.remove(kTmpPath);
            return SaveResult::Failed;
        }
    }
    // 2. The old one aside (rename never overwrites here).
    const bool hadOld = mKernel.fs.exist(kPath);
    if (hadOld) {
        if (mKernel.fs.exist(kBakPath)) {
            mKernel.fs.remove(kBakPath);
        }
        if (!mKernel.fs.rename(kPath, kBakPath)) {
            LOG_ERROR("Stored tracks: cannot move [%s] aside\n", kPath);
            // The old file is untouched. The new one goes: the caller keeps
            // the old store, so a later start must not load it.
            mKernel.fs.remove(kTmpPath);
            return SaveResult::Failed;
        }
    }
    // 3. The new one in place.
    if (!mKernel.fs.rename(kTmpPath, kPath)) {
        LOG_ERROR("Stored tracks: cannot rename [%s] to [%s]\n", kTmpPath, kPath);
        if (hadOld) {
            mKernel.fs.rename(kBakPath, kPath); // put the old one back
        }
        mKernel.fs.remove(kTmpPath); // as above: the old store stands
        return SaveResult::Failed;
    }
    // 4. Tidy up.
    if (hadOld) {
        mKernel.fs.remove(kBakPath);
    }
    LOG_INFO("Stored tracks saved: %u tracks, %u bytes\n", store.count(), static_cast<unsigned>(len));
    return SaveResult::Saved;
}
