/**
 ******************************************************************************
 * @file    RecordingMarker.cpp
 * @brief   Crash-recovery marker for an in-progress FIT recording.
 ******************************************************************************
 */

#include "SDK/Fit/RecordingMarker.hpp"

#include "SDK/Fit/FitWriter.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace SDK::Fit {

namespace {
    // Sibling suffixes for the crash-atomic rotate (see write()).
    constexpr const char* kTmpSuffix = ".tmp";
    constexpr const char* kBakSuffix = ".bak";

    // A sibling path is the marker path plus a 4-char suffix; size the fixed
    // buffer for the longest possible marker path plus room for the suffix + NUL
    // so we never allocate/grow a std::string on the hot flush path.
    constexpr size_t kSiblingBufLen = SDK::Interface::IFileSystem::skMaxPathLen + 8;

    // Build "<base><suffix>" into a fixed buffer (no heap).
    void makeSiblingPath(char* out, size_t outSize, const char* base, const char* suffix)
    {
        snprintf(out, outSize, "%s%s", base, suffix);
    }

    // Read + parse one marker file (path on line 1, decimal offset on line 2).
    // Returns false when the file is absent, unreadable, empty (torn), or
    // unparseable -- the caller then falls back to the .bak copy.
    bool readMarkerFile(SDK::Interface::IFileSystem& fs, const char* path,
                        std::string& fitPath, uint32_t& offset)
    {
        auto marker = fs.file(path);
        if (!marker || !marker->exist() || !marker->open(/*wMode=*/false)) {
            return false;
        }

        char   buff[SDK::Interface::IFileSystem::skMaxPathLen + 16]{};
        size_t br = 0;
        const bool ok = marker->read(buff, sizeof(buff) - 1, br);
        marker->close();
        if (!ok || br == 0) {
            return false;
        }
        buff[br] = '\0';

        const char* nl = std::strchr(buff, '\n');
        if (nl == nullptr) {
            return false;
        }
        fitPath.assign(buff, static_cast<size_t>(nl - buff));
        offset = static_cast<uint32_t>(std::strtoul(nl + 1, nullptr, 10));
        return !fitPath.empty();
    }
}  // namespace

RecordingMarker::RecordingMarker(SDK::Interface::IFileSystem& fs, const char* activityDir)
    : mFs(fs)
    , mMarkerPath(std::string(activityDir != nullptr ? activityDir : "") + "/" + kFileName)
{
}

bool RecordingMarker::write(const char* fitPath, uint32_t offset)
{
    mActiveFitPath = fitPath != nullptr ? fitPath : "";

    char tmpPath[kSiblingBufLen];
    char bakPath[kSiblingBufLen];
    makeSiblingPath(tmpPath, sizeof(tmpPath), mMarkerPath.c_str(), kTmpSuffix);
    makeSiblingPath(bakPath, sizeof(bakPath), mMarkerPath.c_str(), kBakSuffix);

    // 1) Write the new marker to a temp sibling, flush + close. This truncating
    //    open only ever touches the temp, so a crash in the truncate->flush
    //    window can never damage the live primary -- the old good copy (or its
    //    .bak) survives untouched. Mirrors Settings::ManagerBase::write().
    auto tmp = mFs.file(tmpPath);
    if (!tmp || !tmp->open(/*wMode=*/true, /*override=*/true)) {
        return false;
    }

    // Trivial line-based format: .fit path on line 1, decimal offset on line 2.
    char      buff[SDK::Interface::IFileSystem::skMaxPathLen + 16]{};
    const int len = snprintf(buff, sizeof(buff), "%s\n%u\n", mActiveFitPath.c_str(), offset);
    if (len <= 0) {
        tmp->close();
        return false;
    }
    // Defensive clamp: on truncation of an over-long path snprintf returns the
    // would-be length, which can exceed the buffer -- never write past it.
    const size_t writeLen =
        static_cast<size_t>(len) < sizeof(buff) ? static_cast<size_t>(len) : sizeof(buff) - 1;

    size_t bw = 0;
    bool   ok = tmp->write(buff, writeLen, bw) && bw == writeLen;
    ok        = tmp->flush() && ok;   // the marker's own durability point
    tmp->close();
    if (!ok) {
        mFs.remove(tmpPath);
        return false;
    }

    // 2) Publish atomically. Keep a good copy present at every crash point.
    if (mFs.exist(mMarkerPath.c_str())) {
        // Normal update: rotate the current good primary to .bak (FatFs rename
        // fails onto an existing target, so clear any stale .bak first), then
        // promote the temp. A crash after the rotate but before the promote
        // leaves .bak = old good, recovered by read()'s fallback.
        if (mFs.exist(bakPath)) {
            mFs.remove(bakPath);
        }
        mFs.rename(mMarkerPath.c_str(), bakPath);          // primary -> .bak
        return mFs.rename(tmpPath, mMarkerPath.c_str());   // temp -> primary
    }

    // First write (no primary to rotate): promote the temp directly. Any
    // existing .bak is left in place so a valid copy stays present right up to
    // the atomic promote.
    return mFs.rename(tmpPath, mMarkerPath.c_str());
}

bool RecordingMarker::update(uint32_t offset)
{
    if (mActiveFitPath.empty()) {
        return false;
    }
    return write(mActiveFitPath.c_str(), offset);
}

bool RecordingMarker::read(std::string& fitPath, uint32_t& offset) const
{
    // Primary first; if it is missing or torn (0 bytes / unparseable after a
    // crash mid-publish), fall back to the .bak. A recovered .bak offset is a
    // slightly older (previous-flush) but still record-aligned, durable offset
    // -- graceful degradation, not an error.
    if (readMarkerFile(mFs, mMarkerPath.c_str(), fitPath, offset)) {
        return true;
    }
    char bakPath[kSiblingBufLen];
    makeSiblingPath(bakPath, sizeof(bakPath), mMarkerPath.c_str(), kBakSuffix);
    return readMarkerFile(mFs, bakPath, fitPath, offset);
}

void RecordingMarker::remove()
{
    char tmpPath[kSiblingBufLen];
    char bakPath[kSiblingBufLen];
    makeSiblingPath(tmpPath, sizeof(tmpPath), mMarkerPath.c_str(), kTmpSuffix);
    makeSiblingPath(bakPath, sizeof(bakPath), mMarkerPath.c_str(), kBakSuffix);
    mFs.remove(mMarkerPath.c_str());
    mFs.remove(bakPath);
    mFs.remove(tmpPath);
}

RecordingMarker::RecoverResult RecordingMarker::recover()
{
    RecoverResult result;

    std::string fitPath;
    uint32_t    dataEnd = 0;
    if (!read(fitPath, dataEnd)) {
        return result;   // no marker -> nothing to recover
    }

    auto file = mFs.file(fitPath.c_str());
    if (file && file->exist() && FitWriter::recover(*file, dataEnd)) {
        // A successful recover() leaves the file open in write mode, and
        // destroying the handle does NOT close it. Close it here: a leaked
        // write-mode open pins a FatFs lock-table slot until reboot, making
        // every later open/rename/delete of the just-recovered activity fail
        // with FR_LOCKED. (Failed recovers close on every path themselves.)
        file->close();
        result.recovered = true;
        result.path      = fitPath;
    }

    // One-shot: always clear the marker (a missing file or failed recover is
    // given up on cleanly so the next boot does not retry forever).
    remove();
    return result;
}

}  // namespace SDK::Fit
