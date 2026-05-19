/**
 ******************************************************************************
 * @file    SettingsSerializer.cpp
 * @date    08-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Serializes/Deserializes settings to a file.
 ******************************************************************************
 *
 ******************************************************************************
 */
#include "SettingsSerializer.hpp"

#include <cassert>

#include "SDK/JSON/JsonStreamWriter.hpp"
#include "SDK/JSON/JsonStreamReader.hpp"

#define LOG_MODULE_PRX      "SettingsSerializer"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"


static const char* metricToStr(Settings::Intervals::Metric metric)
{
    switch (metric) {
    case Settings::Intervals::Metric::OPEN:     return "OPEN";
    case Settings::Intervals::Metric::TIME:     return "TIME";
    case Settings::Intervals::Metric::DISTANCE: return "DISTANCE";
    default:                                    return "OPEN"; // default
    }
}

static Settings::Intervals::Metric strToMetric(std::string_view str)
{
    if (str == "OPEN")      return Settings::Intervals::Metric::OPEN;
    if (str == "DISTANCE")  return Settings::Intervals::Metric::DISTANCE;
    if (str == "TIME")      return Settings::Intervals::Metric::TIME;

    return Settings::Intervals::Metric::OPEN; // default
}


SettingsSerializer::SettingsSerializer(const SDK::Kernel& kernel,
    const char *pathToFile) :
    mKernel(kernel), mPath(pathToFile)
{
    assert(pathToFile != nullptr);
}

bool SettingsSerializer::save(const Settings &settings)
{
    const char *slash = strrchr(mPath, '/');
    if (slash) {
        char buff[SDK::Interface::IFileSystem::skMaxPathLen] { };
        snprintf(buff, sizeof(buff), "%.*s", static_cast<size_t>(slash - mPath), mPath);
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

    writer.add("version",        settings.version);
    writer.add("auto_pause_en",  settings.autoPauseEn);
    writer.add("phone_notif_en", settings.phoneNotifEn);
    writer.add("alert_distance_id", static_cast<uint8_t>(settings.alertDistanceId));
    writer.add("alert_time_id",     static_cast<uint8_t>(settings.alertTimeId));

    writer.startMap("intervals");
    writer.add("repeats_num",   settings.intervals.repeatsNum);
    writer.add("run_metric",    metricToStr(settings.intervals.runMetric));
    writer.add("run_time",      settings.intervals.runTime);
    writer.add("run_distance",  settings.intervals.runDistance);
    writer.add("rest_metric",   metricToStr(settings.intervals.restMetric));
    writer.add("rest_time",     settings.intervals.restTime);
    writer.add("rest_distance", settings.intervals.restDistance);
    writer.add("warm_up",       settings.intervals.warmUp);
    writer.add("cool_down",     settings.intervals.coolDown);
    writer.endMap();

    writer.endMap();

    file->flush();
    file->close();

    return true;
}

bool SettingsSerializer::load(Settings &settings)
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

    reader.get("version",        settings.version);
    reader.get("auto_pause_en",  settings.autoPauseEn);
    reader.get("phone_notif_en", settings.phoneNotifEn);
    uint8_t distId = static_cast<uint8_t>(Settings::Alerts::Distance::ID_OFF);
    uint8_t timeId = static_cast<uint8_t>(Settings::Alerts::Time::ID_OFF);
    reader.get("alert_distance_id", distId);
    reader.get("alert_time_id",     timeId);
    settings.alertDistanceId = distId < Settings::Alerts::Distance::ID_COUNT
        ? static_cast<Settings::Alerts::Distance::Id>(distId)
        : Settings::Alerts::Distance::ID_OFF;
    settings.alertTimeId = timeId < Settings::Alerts::Time::ID_COUNT
        ? static_cast<Settings::Alerts::Time::Id>(timeId)
        : Settings::Alerts::Time::ID_OFF;

    reader.get("intervals.repeats_num",   settings.intervals.repeatsNum);
    reader.get("intervals.run_time",      settings.intervals.runTime);
    reader.get("intervals.run_distance",  settings.intervals.runDistance);
    reader.get("intervals.rest_time",     settings.intervals.restTime);
    reader.get("intervals.rest_distance", settings.intervals.restDistance);
    reader.get("intervals.warm_up",       settings.intervals.warmUp);
    reader.get("intervals.cool_down",     settings.intervals.coolDown);

    std::string_view metric;
    if (reader.get("intervals.run_metric", metric)) {
        settings.intervals.runMetric = strToMetric(metric);
    }
    if (reader.get("intervals.rest_metric", metric)) {
        settings.intervals.restMetric = strToMetric(metric);
    }

    delete[] buffer;

    return true;
}
