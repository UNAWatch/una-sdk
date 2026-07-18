/**
 ******************************************************************************
 * @file    RecordingMarker.hpp
 * @brief   Crash-recovery marker for an in-progress FIT recording.
 *
 * An activity's .fit filename encodes its start time, which the next boot does
 * not know, so a torn recording cannot be located after a power loss from the
 * filename alone. This marker is a single fixed-path file, one per activity
 * directory, holding the path of the currently-recording .fit and the byte
 * offset one-past the last record-complete, flushed data record. Together they
 * let the next boot find the interrupted file and finalize it at a clean record
 * boundary via SDK::Fit::FitWriter::recover().
 *
 * The on-disk format is deliberately trivial and robust: the .fit path on the
 * first line, the decimal offset on the second.
 *
 * write()/update() are crash-atomic: the new marker is staged to a ".tmp"
 * sibling (flushed + closed) and then published by rotating the previous good
 * primary to a ".bak" and renaming the temp into place. A crash at any point
 * leaves either the new primary or the previous ".bak" fully intact, so read()
 * never gets a torn/empty marker that would orphan a recoverable .fit; it falls
 * back to the ".bak" (a slightly older but still record-aligned, durable
 * offset). Mirrors Settings::ManagerBase's proven rotate sequence.
 *
 * All logic here is activity/app independent -- it is parameterized only by an
 * IFileSystem and the activity directory -- so every FIT-writing app reuses the
 * same marker I/O and the same one-call recover orchestration.
 ******************************************************************************
 */

#ifndef __SDK_FIT_RECORDING_MARKER_HPP
#define __SDK_FIT_RECORDING_MARKER_HPP

#include "SDK/Interfaces/IFileSystem.hpp"

#include <cstdint>
#include <string>

namespace SDK::Fit {

class RecordingMarker {
public:
    /// Fixed marker file name, placed inside the activity directory.
    static constexpr const char* kFileName = ".recording";

    /// Result of a recover() attempt.
    struct RecoverResult {
        bool        recovered = false;  ///< true iff an interrupted .fit was finalized
        std::string path;               ///< the recovered .fit path (valid iff recovered)
    };

    /// @param fs           File system used to read/write the marker + .fit.
    /// @param activityDir  Directory the marker lives in (e.g. "Activity").
    RecordingMarker(SDK::Interface::IFileSystem& fs, const char* activityDir);

    /// (Re)write the marker = { fitPath, offset }. Remembers @p fitPath so
    /// later update() calls need only the new offset. Flushed before return.
    bool write(const char* fitPath, uint32_t offset);

    /// Rewrite the marker keeping the .fit path from the last write(), moving
    /// only the offset. Fails if write() was never called.
    bool update(uint32_t offset);

    /// Read the marker into @p fitPath / @p offset. Returns false when the
    /// marker is absent or unreadable.
    bool read(std::string& fitPath, uint32_t& offset) const;

    /// Delete the marker (no-op if absent).
    void remove();

    /// Full recover orchestration: if the marker exists, open the .fit it names
    /// and finalize it via FitWriter::recover(file, offset). The marker is
    /// removed afterwards (recovery is one-shot; give up cleanly on a missing
    /// file or a failed recover) -- EXCEPT when the finalized file's close()
    /// fails: the bytes are durable but the handle (and its FatFs lock) stays
    /// held, so the marker is kept and failure reported; the next boot retries
    /// (FitWriter::recover() is idempotent on an already-finalized file).
    /// Safe to call with no marker present.
    RecoverResult recover();

private:
    SDK::Interface::IFileSystem& mFs;
    std::string                  mMarkerPath;   ///< "<activityDir>/.recording"
    std::string                  mActiveFitPath;///< last path passed to write()
};

}  // namespace SDK::Fit

#endif  // __SDK_FIT_RECORDING_MARKER_HPP
