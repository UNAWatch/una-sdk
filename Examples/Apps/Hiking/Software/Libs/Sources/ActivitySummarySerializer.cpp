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
        snprintf(buff, sizeof(buff), "%.*s", static_cast<size_t>(slash - mPath), mPath);
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

    writer.add("utc",       static_cast<uint32_t>(summary.utc));
    writer.add("time",      static_cast<uint32_t>(summary.time));
    writer.add("distance",  summary.distance);
    writer.add("speed_avg", summary.speedAvg);
    writer.add("steps",     summary.steps);
    writer.add("elevation", summary.elevation);
    writer.add("pace_avg",  summary.paceAvg);
    writer.add("hr_max",    summary.hrMax);
    writer.add("hr_avg",    summary.hrAvg);

    const uint8_t* points = reinterpret_cast<const uint8_t*>(summary.map.points.data());
    writer.addHexString("map", points, summary.map.points.size() * 2);

    writer.add("lap_count", static_cast<uint32_t>(summary.laps.size()));
    writer.startArray("laps");
    for (const LapSummary& lap : summary.laps) {
        writer.startMap();
        writer.add("dur",   static_cast<uint32_t>(lap.duration));
        writer.add("dist",  lap.distance);
        writer.add("steps", lap.steps);
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
    if (fileSize == 0) {
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
    bool status = file->read(buffer, fileSize, read) && (read == fileSize);

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
    reader.get("utc",  summary.utc);
#endif
    reader.get("distance",  summary.distance);
    reader.get("speed_avg", summary.speedAvg);
    reader.get("steps",     summary.steps);
    reader.get("elevation", summary.elevation);
    reader.get("pace_avg",  summary.paceAvg);
    reader.get("hr_max",    summary.hrMax);
    reader.get("hr_avg",    summary.hrAvg);

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

        snprintf(query, sizeof(query), "laps[%" PRIu32 "].dist", i);
        reader.get(query, lap.distance);

        snprintf(query, sizeof(query), "laps[%" PRIu32 "].steps", i);
        reader.get(query, lap.steps);

        summary.laps.push_back(lap);
    }

    // Track map as HEX-String
    const char* hexStr = nullptr;
    size_t hexStrLen = 0;

    if (reader.get("map", hexStr, hexStrLen) && hexStrLen % 4 == 0) {
        summary.map.points.reserve(hexStrLen / 4);
        for (size_t i = 0; i < hexStrLen / 4; i++) {
            SDK::TrackMapScreen::Point point{ };
            char xstr[3] = { hexStr[i * 4], hexStr[i * 4 + 1], 0 };
            char ystr[3] = { hexStr[i * 4 + 2], hexStr[i * 4 + 3], 0 };
            point.x = static_cast<uint8_t>(strtol(xstr, nullptr, 16));
            point.y = static_cast<uint8_t>(strtol(ystr, nullptr, 16));
            summary.map.points.push_back(point);
        }
    }

    delete[] buffer;

    return true;
}
