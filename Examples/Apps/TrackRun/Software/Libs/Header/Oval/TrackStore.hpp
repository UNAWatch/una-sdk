/*******************************************************************************
 * @file   TrackStore.hpp
 * @brief  Oval: up to 10 calibrated tracks, saved and recognised.
 *
 *         Tracks are named by number ("Track 2"). Numbers are stable:
 *         forgetting one does not renumber the others, and a new track takes
 *         the lowest free number. When an 11th is saved, the least recently
 *         used one is replaced and its number reused.
 *
 *         Each record holds the model (centre latitude and longitude, heading,
 *         bend radius, running direction, mean altitude), the fit quality,
 *         when it was first saved and last used (UNIX seconds passed in by
 *         the caller: Oval reads no clock), how often it was used, and a
 *         format version.
 *
 *         The store reads and writes a small text format (docs/OVAL.md),
 *         one line per track, each with a CRC, so a damaged
 *         file loses only its damaged lines. File I/O is the service's job:
 *         write to a temporary file, move the old one aside, then rename the
 *         new one into its place.
 *
 *         No heap, no exceptions, no SDK headers.
 ******************************************************************************/

#ifndef OVAL_TRACK_STORE_HPP
#define OVAL_TRACK_STORE_HPP

#include "Oval/Config.hpp"
#include "Oval/TrackModel.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>

namespace Oval {

/** How well a calibration fitted (Calibrator::FitInfo). */
struct FitQuality {
    float scaleM = 0.0f; ///< Robust residual scale
    float shapeM = 0.0f; ///< Noise-corrected systematic residual
};

/** What a run means for the store, from Tracker::storeOutcome(). */
struct StoreOutcome {
    enum class Kind : uint8_t {
        None,      ///< Nothing to do
        New,       ///< Save model as a new track
        Updated,   ///< The visit agreed with the stored track: blend model into it
        Replaced,  ///< The stored track no longer fits: replace it with model, keep its number
        Unchanged, ///< The stored track was used: record the use only
    };
    Kind       kind = Kind::None;
    uint8_t    lane = 0;    ///< The lane last run on the track, 0 if unknown
    TrackModel model;   ///< The run's locked fit, with latitude, longitude and altitude
    FitQuality quality;
};

/** One stored track. */
struct TrackRecord {
    double   lat         = 0.0; ///< Centre, degrees
    double   lon         = 0.0;
    float    headingDeg  = 0.0f; ///< Heading of the straights, [0, 180)
    float    kerbRadiusM = 36.5f;
    float    altM        = std::numeric_limits<float>::quiet_NaN(); ///< Mean altitude (NaN if unknown)
    float    scaleM      = 0.0f; ///< Fit quality
    float    shapeM      = 0.0f;
    uint32_t firstSavedS = 0;    ///< UNIX seconds
    uint32_t lastUsedS   = 0;
    uint16_t fits        = 0;    ///< Visits blended into the model (its weight)
    uint16_t uses        = 0;    ///< Runs that used it
    uint8_t  number      = 0;    ///< 1 to TrackStore::kMaxTracks; 0 = empty slot
    uint8_t  version     = 0;    ///< Format version it was written with
    uint8_t  lane        = 0;    ///< Lane last run there, 0 if unknown
    bool     clockwise   = false; ///< The direction it was last run

    bool       isValid() const noexcept { return number != 0; }
    TrackModel model() const noexcept;
};

class TrackStore {
public:
    static constexpr uint8_t  kMaxTracks          = 10;
    static constexpr uint8_t  kFormatVersion      = 1;
    static constexpr size_t   kMaxSerialisedBytes = 2560; ///< A full store fits in this
    static constexpr size_t   kMaxLineBytes       = 400;  ///< Longer lines are not tracks

    struct SaveResult {
        uint8_t number   = 0; ///< The track's number (0: not saved)
        uint8_t replaced = 0; ///< The track that went to make room, 0 if none
    };

    /** What apply() did. */
    struct Applied {
        StoreOutcome::Kind kind     = StoreOutcome::Kind::None;
        uint8_t            number   = 0; ///< The track saved, updated or touched
        uint8_t            replaced = 0; ///< A least recently used track that went
        bool               changed  = false; ///< The store changed: save the file
    };

    /** What load() found. Every line that is not loaded is counted. */
    struct LoadReport {
        bool     headerOk     = false; ///< A valid header line came first
        uint8_t  version      = 0;     ///< The header's format version (0 if none)
        bool     newerVersion = false; ///< Written by a later version: loaded what could be
        uint8_t  expected     = 0;     ///< Tracks the header says the file holds (0 if none)
        uint8_t  loaded       = 0;
        uint16_t lines        = 0;     ///< Lines seen, complete or not
        uint16_t truncated    = 0;     ///< A last line with no end (a cut-off file)
        uint16_t badChecksum  = 0;     ///< CRC missing or wrong (garbled)
        uint16_t badField     = 0;     ///< A required field missing or out of range
        uint16_t duplicate    = 0;     ///< A number already loaded
        uint16_t otherLines   = 0;     ///< Not track lines (blank, or unknown to this version)

        /** Track lines dropped. */
        uint16_t dropped() const noexcept { return truncated + badChecksum + badField + duplicate; }
    };

    explicit TrackStore(const Config& cfg = Config()) noexcept;

    void setConfig(const Config& cfg) noexcept;
    void clear() noexcept;

    uint8_t            count() const noexcept;
    const TrackRecord* find(uint8_t number) const noexcept;

    /** The stored track whose centre is nearest, if within storeNearM.
     *  distM, if given, gets its distance. skip, if not 0, is a track number
     *  left out (the next nearest after a wrong pick). */
    const TrackRecord* findNear(double lat, double lon, float* distM = nullptr, uint8_t skip = 0) const noexcept;

    /** Adds a track (the model must have its latitude and longitude). When
     *  the store is full, the least recently used track is replaced, other
     *  than those in keep (bit n - 1 for track n). */
    SaveResult saveNew(const TrackModel& model, const FitQuality& q, uint32_t nowS, uint16_t keep = 0) noexcept;

    /** Replaces a track's model, keeping its number. Counts as a use. */
    bool update(uint8_t number, const TrackModel& model, const FitQuality& q, uint32_t nowS) noexcept;

    /** Blends a visit that agreed into a track, weighted by the visits so
     *  far (up to storeBlendMaxWeight). Counts as a use. */
    bool blend(uint8_t number, const TrackModel& model, const FitQuality& q, uint32_t nowS) noexcept;

    /** Records a use. */
    bool touch(uint8_t number, uint32_t nowS) noexcept;

    /** Removes one track. Others keep their numbers. */
    bool forget(uint8_t number) noexcept;

    /** All tracks, ordered by number. Returns how many. */
    uint8_t list(const TrackRecord* out[kMaxTracks]) const noexcept;

    /** Applies the end of a run: suppliedNumber is the stored track given to
     *  Tracker::start() (0 if none). A new track replaces none in keep
     *  (saveNew()). */
    Applied apply(const StoreOutcome& outcome, uint8_t suppliedNumber, uint32_t nowS, uint16_t keep = 0) noexcept;

    /** Applies a run's n outcomes (one a track visited), out[i] for visit
     *  i: the visits to stored tracks first, so a track used in the run is
     *  touched before a new one takes the least recently used place, and a
     *  new one never takes the place of a track the run used (with the
     *  clock not set every date is 0, so that is not the dates alone). */
    void applyRun(const StoreOutcome* outcomes, const uint8_t* suppliedNumbers, uint8_t n, uint32_t nowS,
                  Applied* out) noexcept;

    /** Writes the store as text. Returns its length, or 0 if cap is too
     *  small (kMaxSerialisedBytes is always enough). Not NUL-terminated. */
    size_t serialise(char* buf, size_t cap) const noexcept;

    /** Replaces the store with what a text holds. Damaged lines are dropped
     *  and counted; never fails. */
    LoadReport load(const char* text, size_t len) noexcept;

private:
    TrackRecord* slot(uint8_t number) noexcept;
    bool         fill(TrackRecord& r, const TrackModel& model, const FitQuality& q) const noexcept;

    TrackRecord mRecs[kMaxTracks]; ///< Track n is in mRecs[n - 1]
    float       mNearM     = 150.0f;
    uint8_t     mBlendMax  = 9;
};

} // namespace Oval

#endif // OVAL_TRACK_STORE_HPP
