/**
 ******************************************************************************
 * @file    StrideLut.cpp
 * @brief   Implementation of the shared stride lookup table.
 ******************************************************************************
 */

#include "SDK/Calibration/StrideLut.hpp"

#include <cmath>
#include <cstdio>
#include <memory>
#include <new>

#include "SDK/JSON/JsonStreamReader.hpp"

namespace SDK::Calibration
{

// --- Bin layout helpers ------------------------------------------------------

float StrideLut::binCentreSpm(size_t index)
{
    return Config::kBinBaseSpm + Config::kBinWidthSpm * static_cast<float>(index)
           + Config::kBinWidthSpm / 2.0f;
}

size_t StrideLut::binIndexForCadence(float cadenceSpm)
{
    const float rel = (cadenceSpm - Config::kBinBaseSpm) / Config::kBinWidthSpm;
    if (rel <= 0.0f) {
        return 0;
    }
    const long idx = static_cast<long>(std::floor(rel));
    if (idx >= static_cast<long>(Config::kBinCount - 1)) {
        return Config::kBinCount - 1;
    }
    return static_cast<size_t>(idx);
}

void StrideLut::clear()
{
    for (StrideBin &b : mBins) {
        b = StrideBin {};
    }
}

// --- Phase-2 gate read path --------------------------------------------------

size_t StrideLut::validBinCount() const
{
    size_t n = 0;
    for (const StrideBin &b : mBins) {
        if (b.isValid()) {
            ++n;
        }
    }
    return n;
}

float StrideLut::totalCalibrationDistanceM() const
{
    float total = 0.0f;
    for (const StrideBin &b : mBins) {
        total += b.total_distance_m;
    }
    return total;
}

bool StrideLut::readyForPhase2() const
{
    return validBinCount() >= Config::kOutdoorLutMinValidBins;
}

bool StrideLut::readyForOutdoorEstimate() const
{
    return validBinCount() >= Config::kOutdoorLutMinValidBinsEstimate;
}

// --- Persistence -------------------------------------------------------------

bool StrideLut::parseBuffer(const char *data, size_t len)
{
    SDK::JsonStreamReader reader(data, len);
    if (!reader.validate()) {
        return false;  // malformed → caller decides whether to back up
    }

    int32_t version = 0;
    if (!reader.get("version", version)) {
        return false;  // cannot read version → treat as corrupt
    }

    // version < current with no registered migration → start fresh, no backup
    // (bins were already cleared by the caller). Equivalent to an empty load.
    if (version < kStoreVersion) {
        return true;
    }

    // version >= current: load known fields; unknown keys are ignored by the
    // query reader, giving forward compatibility for newer files.
    size_t arrayLen = 0;
    if (!reader.getArrayLength("bins", arrayLen)) {
        return true;  // no bins array → empty store, still valid
    }

    // Bound the iteration: the schema has exactly kBinCount bins, so a
    // corrupt-but-valid file claiming a huge array length can't spin the loop.
    const size_t binLimit = (arrayLen < Config::kBinCount) ? arrayLen
                                                           : Config::kBinCount;
    for (size_t k = 0; k < binLimit; ++k) {
        char query[64];

        float centre = 0.0f;
        std::snprintf(query, sizeof(query), "bins[%d].centre_spm",
                      static_cast<int>(k));
        if (!reader.get(query, centre)) {
            continue;  // entry without a centre — skip
        }

        const long idx =
            std::lround((centre - binCentreSpm(0)) / Config::kBinWidthSpm);
        if (idx < 0 || idx >= static_cast<long>(Config::kBinCount)) {
            continue;  // centre outside the known range — ignore
        }
        // Accept only exact on-grid centres. An off-grid value (e.g. 83 when the
        // grid is 80/84/88…) signals a corrupt store; aliasing it into the
        // nearest bin would silently shift persisted totals into the wrong
        // cadence bucket and skew LUT/phase-2 readiness, so reject it here.
        // Centres are whole-number SPM persisted as integers, so the tolerance
        // only needs to absorb float round-trip noise — keep it tight.
        constexpr float kCentreMatchEpsSpm = 0.01f;
        if (std::fabs(centre - binCentreSpm(static_cast<size_t>(idx))) >
            kCentreMatchEpsSpm) {
            continue;
        }
        StrideBin &b = mBins[static_cast<size_t>(idx)];

        // Missing keys leave the default (zero) value (backward compatibility).
        std::snprintf(query, sizeof(query), "bins[%d].total_distance_m",
                      static_cast<int>(k));
        reader.get(query, b.total_distance_m);
        std::snprintf(query, sizeof(query), "bins[%d].total_steps",
                      static_cast<int>(k));
        reader.get(query, b.total_steps);
        std::snprintf(query, sizeof(query), "bins[%d].sample_count",
                      static_cast<int>(k));
        reader.get(query, b.sample_count);

        // Reject implausible persisted values (non-finite or negative): a
        // syntactically valid but corrupt file must not inject bad state that
        // would skew SL / phase-2 readiness.
        if (!std::isfinite(b.total_distance_m) || b.total_distance_m < 0.0f ||
            !std::isfinite(b.total_steps)      || b.total_steps < 0.0f ||
            !std::isfinite(b.sample_count)     || b.sample_count < 0.0f) {
            b = StrideBin {};
        }
    }

    return true;
}

bool StrideLut::loadFromFile(SDK::Interface::IFileSystem &fs, const char *path)
{
    clear();

    const char *p = (path != nullptr) ? path : kDefaultPath;

    std::unique_ptr<SDK::Interface::IFile> file = fs.file(p);
    if (!file || !file->exist()) {
        return false;  // absent → all-zero
    }
    if (!file->open(false, false)) {
        return false;  // cannot open → all-zero
    }

    const size_t size = file->size();
    if (size == 0 || size > kMaxStoreBytes) {
        // Empty or implausibly large: start all-zero. No .bak (read-only owner).
        file->close();
        return false;
    }

    char *buffer = new (std::nothrow) char[size];
    if (buffer == nullptr) {
        file->close();
        return false;
    }

    size_t read = 0;
    const bool readOk = file->read(buffer, size, read) && (read == size);
    file->close();

    bool ok = false;
    if (readOk && parseBuffer(buffer, size)) {
        ok = true;
    } else {
        clear();  // malformed / read error → all-zero
    }

    delete[] buffer;
    return ok;
}

} // namespace SDK::Calibration
