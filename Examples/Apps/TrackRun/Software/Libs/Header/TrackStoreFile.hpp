/**
 ******************************************************************************
 * @file    TrackStoreFile.hpp
 * @brief   The stored tracks file (tracks.txt in the app folder): loads it into
 *          an Oval::TrackStore and saves it atomically (the format is in
 *          docs/OVAL.md).
 *
 *          Saving writes tracks.tmp, moves tracks.txt aside to tracks.bak,
 *          renames tracks.tmp to tracks.txt and removes tracks.bak. Rename
 *          does not overwrite on the watch (FatFs) or in the Windows
 *          simulator, hence the move aside. A crash at any point leaves a
 *          whole file: with tracks.txt missing, tracks.tmp (written in full
 *          before tracks.txt moves) is the newer store, then tracks.bak. A
 *          file that loads no track is passed over for one that does (every
 *          line carries a CRC, so a cut-off file loses only its tail).
 *
 *          A file written by a newer version is never saved over: that would
 *          drop the keys this version does not know.
 ******************************************************************************
 */

#ifndef TRACK_STORE_FILE_HPP
#define TRACK_STORE_FILE_HPP

#include "Oval/TrackStore.hpp"
#include "SDK/Kernel/Kernel.hpp"

#include <cstddef>

class TrackStoreFile {
public:
    enum class SaveResult : uint8_t { Saved, SkippedNewer, Failed };

    explicit TrackStoreFile(SDK::Kernel& kernel) : mKernel(kernel) {}

    /** Replaces store with the file's tracks (empty if there is no file).
     *  fromPath is set to the file read, or nullptr. */
    Oval::TrackStore::LoadReport load(Oval::TrackStore& store, const char** fromPath = nullptr);

    SaveResult save(const Oval::TrackStore& store);

    bool newerVersion() const { return mNewer; }

    /** The store as text (header and one line per track), for the GPS trace's
     *  meta lines. Returns the length; buf is the internal buffer. */
    size_t serialise(const Oval::TrackStore& store, const char*& buf);

    static constexpr const char* kPath    = "tracks.txt";
    static constexpr const char* kTmpPath = "tracks.tmp";
    static constexpr const char* kBakPath = "tracks.bak";

private:
    bool readAll(const char* path, size_t& len);
    /** Moves a recovered side file (mRecovered) to kPath, a damaged main
     *  file giving way. True when there is none left to move. */
    bool promoteRecovered();

    SDK::Kernel& mKernel;
    bool         mNewer = false;
    const char*  mRecovered = nullptr; ///< A side file the tracks were loaded from, not yet in place
    char         mBuf[Oval::TrackStore::kMaxSerialisedBytes] = {};
};

#endif // TRACK_STORE_FILE_HPP
