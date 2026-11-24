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

/**
 * Read the run alerts, falling back to the defaults for anything missing or
 * out of range: a Run/Walk time to its default, a pace or heart-rate limit to
 * Off. An inverted pair (Too Fast not quicker than Too Slow, HR Low not below
 * HR High) turns both its limits Off.
 */
static void loadRunAlerts(const SDK::JsonStreamReader& reader, Settings::RunAlerts& out)
{
    using RA = Settings::RunAlerts;

    // Defaults first, so a file from before these keys loads the defaults
    // whatever the struct held.
    out = RA{};

    uint32_t runSec  = out.runWalkRunSec;
    uint32_t walkSec = out.runWalkWalkSec;
    float    fast    = 0.0f;
    float    slow    = 0.0f;
    uint32_t high    = 0;
    uint32_t low     = 0;

    reader.get("run_alerts.run_walk_en",        out.runWalkEn);
    reader.get("run_alerts.run_walk_run_time",  runSec);
    reader.get("run_alerts.run_walk_walk_time", walkSec);
    reader.get("run_alerts.pace_fast",          fast);
    reader.get("run_alerts.pace_slow",          slow);
    reader.get("run_alerts.hr_high",            high);
    reader.get("run_alerts.hr_low",             low);
    reader.get("run_alerts.back_in_tap",        out.backInTap);

    const auto timeOk = [](uint32_t s) { return s >= RA::kRunWalkMinSec && s <= RA::kRunWalkMaxSec; };
    // Written so that a NaN fails it too.
    const auto paceOk = [](float p) { return p >= RA::kPaceMinSecPerKm && p <= RA::kPaceMaxSecPerKm; };
    const auto hrOk   = [](uint32_t b) { return b >= RA::kHrMinBpm && b <= RA::kHrMaxBpm; };

    if (timeOk(runSec))  { out.runWalkRunSec  = runSec; }
    if (timeOk(walkSec)) { out.runWalkWalkSec = walkSec; }
    out.paceFastSecPerKm = paceOk(fast) ? fast : 0.0f;
    out.paceSlowSecPerKm = paceOk(slow) ? slow : 0.0f;
    out.hrHighBpm        = hrOk(high) ? static_cast<uint8_t>(high) : 0;
    out.hrLowBpm         = hrOk(low)  ? static_cast<uint8_t>(low)  : 0;

    if (out.paceFastSecPerKm > 0.0f && out.paceSlowSecPerKm > 0.0f
        && out.paceFastSecPerKm >= out.paceSlowSecPerKm) {
        out.paceFastSecPerKm = 0.0f;
        out.paceSlowSecPerKm = 0.0f;
    }
    if (out.hrHighBpm > 0 && out.hrLowBpm > 0 && out.hrLowBpm >= out.hrHighBpm) {
        out.hrHighBpm = 0;
        out.hrLowBpm  = 0;
    }
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

    writer.add("version",        settings.version);
    writer.add("auto_pause_en",  settings.autoPauseEn);
    writer.add("phone_notif_en", settings.phoneNotifEn);
    writer.add("calib_trace_en", settings.calibTraceEn);
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
    writer.add("last_rest",     settings.intervals.lastRest);
    writer.endMap();

    writer.startMap("run_alerts");
    writer.add("run_walk_en",        settings.runAlerts.runWalkEn);
    writer.add("run_walk_run_time",  settings.runAlerts.runWalkRunSec);
    writer.add("run_walk_walk_time", settings.runAlerts.runWalkWalkSec);
    writer.add("pace_fast",          settings.runAlerts.paceFastSecPerKm);
    writer.add("pace_slow",          settings.runAlerts.paceSlowSecPerKm);
    writer.add("hr_high",            settings.runAlerts.hrHighBpm);
    writer.add("hr_low",             settings.runAlerts.hrLowBpm);
    writer.add("back_in_tap",        settings.runAlerts.backInTap);
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
    settings.calibTraceEn = false;  // deterministic default for legacy files
    reader.get("calib_trace_en", settings.calibTraceEn);
    uint8_t distId = static_cast<uint8_t>(Settings::Alerts::Distance::ID_DEFAULT);
    uint8_t timeId = static_cast<uint8_t>(Settings::Alerts::Time::ID_OFF);
    reader.get("alert_distance_id", distId);
    reader.get("alert_time_id",     timeId);
    settings.alertDistanceId = distId < Settings::Alerts::Distance::ID_COUNT
        ? static_cast<Settings::Alerts::Distance::Id>(distId)
        : Settings::Alerts::Distance::ID_DEFAULT;
    settings.alertTimeId = timeId < Settings::Alerts::Time::ID_COUNT
        ? static_cast<Settings::Alerts::Time::Id>(timeId)
        : Settings::Alerts::Time::ID_OFF;

    // Distance and time auto-laps are mutually exclusive. Normalize a legacy
    // file that has both enabled by keeping distance (which getLapDivSource()
    // prefers) and clearing time, matching the settings-menu invariant.
    if (settings.alertDistanceId != Settings::Alerts::Distance::ID_OFF) {
        settings.alertTimeId = Settings::Alerts::Time::ID_OFF;
    }

    reader.get("intervals.repeats_num",   settings.intervals.repeatsNum);
    reader.get("intervals.run_time",      settings.intervals.runTime);
    reader.get("intervals.run_distance",  settings.intervals.runDistance);
    reader.get("intervals.rest_time",     settings.intervals.restTime);
    reader.get("intervals.rest_distance", settings.intervals.restDistance);
    reader.get("intervals.warm_up",       settings.intervals.warmUp);
    reader.get("intervals.cool_down",     settings.intervals.coolDown);
    reader.get("intervals.last_rest",     settings.intervals.lastRest);

    std::string_view metric;
    if (reader.get("intervals.run_metric", metric)) {
        settings.intervals.runMetric = strToMetric(metric);
    }
    if (reader.get("intervals.rest_metric", metric)) {
        settings.intervals.restMetric = strToMetric(metric);
    }

    loadRunAlerts(reader, settings.runAlerts);

    delete[] buffer;

    return true;
}
