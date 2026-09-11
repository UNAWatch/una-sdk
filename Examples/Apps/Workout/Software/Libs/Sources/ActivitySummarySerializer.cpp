/**
 ******************************************************************************
 * @file    ActivitySummarySerializer.cpp
 * @date    08-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Serializes/Deserializes summary data to a file.
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "ActivitySummarySerializer.hpp"

#include <cassert>
#include <cinttypes>
#include <cstdlib>

#include "SDK/JSON/JsonStreamWriter.hpp"
#include "SDK/JSON/JsonStreamReader.hpp"

#define LOG_MODULE_PRX      "ActivitySummarySerializer"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

ActivitySummarySerializer::ActivitySummarySerializer(const SDK::Kernel& kernel,
    const char* pathToFile) :
    mKernel(kernel), mPath(pathToFile)
{
    assert(pathToFile != nullptr);
}

bool ActivitySummarySerializer::save(const ActivitySummary& summary)
{
    const char* slash = strrchr(mPath, '/');
    if (slash) {
        char buff[SDK::Interface::IFileSystem::skMaxPathLen]{ };
        snprintf(buff, sizeof(buff), "%.*s", static_cast<int>(slash - mPath), mPath);
        // Create dir
        if (!mKernel.fs.mkdir(buff)) {
            return false;
        }
    }

    std::unique_ptr<SDK::Interface::IFile> file = mKernel.fs.file(mPath);

    if (!file) {
        return false;
    }

    if (!file->open(true, true)) {
        file.reset();
        return false;
    }

    SDK::JsonStreamWriter writer(file.get());

    writer.startMap();

    writer.add("utc", static_cast<uint32_t>(summary.utc));
    writer.add("time", static_cast<uint32_t>(summary.time));
    writer.add("hr_max", summary.hrMax);
    writer.add("hr_avg", summary.hrAvg);
    writer.add("calories", summary.calories);
    writer.add("resting_calories", summary.restingCalories);
    writer.add("active_calories", summary.activeCalories);

    writer.startArray("zone_times_sec");
    for (const std::time_t sec : summary.zoneTimeSec) {
        writer.add(static_cast<uint32_t>(sec));
    }
    writer.endArray();

    writer.add("lap_count", static_cast<uint32_t>(summary.laps.size()));
    writer.startArray("laps");
    for (const LapSummary& lap : summary.laps) {
        writer.startMap();
        writer.add("dur", static_cast<uint32_t>(lap.duration));
        writer.add("hr_avg", lap.hrAvg);
        writer.add("hr_max", lap.hrMax);
        writer.endMap();
    }
    writer.endArray();

    writer.endMap();

    file->flush();
    file->close();

    return true;
}

bool ActivitySummarySerializer::load(ActivitySummary& summary)
{
    std::unique_ptr<SDK::Interface::IFile> file = mKernel.fs.file(mPath);

    if (!file) {
        return false;
    }

    if (!file->exist()) {
        file.reset();
        return false;
    }

    if (!file->open(false, false)) {
        file.reset();
        return false;
    }

    size_t fileSize = file->size();
    if (fileSize == 0) { // Check for adequate size
        file->close();
        file.reset();
        return false;
    }

    char* buffer = new (std::nothrow)char[fileSize];
    if (buffer == nullptr) {
        file->close();
        file.reset();
        return false;
    }

    size_t read = 0;
    bool status = (file->read(buffer, fileSize, read) || read != fileSize);

    file->close();
    file.reset();

    if (!status) {
        delete[] buffer;
        return false;
    }

    SDK::JsonStreamReader reader(buffer, fileSize);

    if (!reader.validate()) {
        LOG_ERROR("JSON is invalid\n");
        delete[] buffer;
        return false;
    }

    // If any fields are missing, just ignore it.

#if defined(SIMULATOR)
#if defined(_USE_32BIT_TIME_T)
    uint32_t tmp;
#else
    uint64_t tmp;
#endif
    reader.get("time", tmp);
    summary.time = static_cast<time_t>(tmp);
    reader.get("utc", tmp);
    summary.utc = static_cast<time_t>(tmp);
#else
    reader.get("time", summary.time);
    reader.get("utc", summary.utc);
#endif
    reader.get("hr_max", summary.hrMax);
    reader.get("hr_avg", summary.hrAvg);
    summary.calories = 0.0f;
    reader.get("calories", summary.calories);

    summary.restingCalories = 0.0f;
    summary.activeCalories  = 0.0f;
    const bool hasRestingCal = reader.get("resting_calories", summary.restingCalories);
    const bool hasActiveCal  = reader.get("active_calories", summary.activeCalories);
    // Older summaries only stored total "calories"; treat it as active for the split UI.
    if (!hasRestingCal && !hasActiveCal && summary.calories > 0.0f) {
        summary.activeCalories = summary.calories;
    }

    // Zone times. Missing in pre-zones summaries -> leave zeros.
    for (size_t i = 0; i < std::size(summary.zoneTimeSec); ++i) {
        summary.zoneTimeSec[i] = 0;
        char query[32];
        snprintf(query, sizeof(query), "zone_times_sec[%zu]", i);
        uint32_t sec = 0;
        if (reader.get(query, sec)) {
            summary.zoneTimeSec[i] = static_cast<std::time_t>(sec);
        }
    }

    // Laps
    uint32_t lapCount = 0;
    reader.get("lap_count", lapCount);
    summary.laps.clear();
    summary.laps.reserve(lapCount);
    for (uint32_t i = 0; i < lapCount; ++i) {
        char query[32];
        LapSummary lap{};

        uint32_t dur = 0;
        snprintf(query, sizeof(query), "laps[%" PRIu32 "].dur", i);
        reader.get(query, dur);
        lap.duration = static_cast<time_t>(dur);

        snprintf(query, sizeof(query), "laps[%" PRIu32 "].hr_avg", i);
        reader.get(query, lap.hrAvg);

        snprintf(query, sizeof(query), "laps[%" PRIu32 "].hr_max", i);
        reader.get(query, lap.hrMax);

        summary.laps.push_back(lap);
    }

    delete[] buffer;

    return true;
}
