#include "AlarmManager.hpp"

#define LOG_MODULE_PRX      "AlarmManager"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/JSON/JsonStreamReader.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

#include <algorithm>


AlarmManager::AlarmManager(const SDK::Kernel& kernel)
    : mKernel(kernel)
{
    mAlarms.reserve(kInitialCount);
    mSnoozedAlarms.reserve(kMaxSnoozes);
    mFiredAlarms.reserve(kInitialCount / 4);
}

AlarmManager::~AlarmManager()
{}


void AlarmManager::load()
{
    loadFromFile(mAlarms);

    LOG_DEBUG("Alarms loaded\n");
    dump(mAlarms);

    // Snoozes outlive a restart, but only as far as their alarm does: one whose
    // alarm has been deleted, or switched off by the user, is not owed a ring.
    loadSnoozesFromFile();
    const size_t restored = mSnoozedAlarms.size();
    removeObsoleteSnoozedAlarms();
    if (mSnoozedAlarms.size() != restored) {
        mSnoozesDirty = true;
    }
    LOG_INFO("Restored %u pending snooze(s)\n",
        static_cast<unsigned>(mSnoozedAlarms.size()));
    persistSnoozesIfDirty();

    if (mObserver) {
        mObserver->onListChanged(mAlarms);
    }
}

uint32_t AlarmManager::execute(const std::tm& tmNow, std::time_t nowUtc)
{
    checkAlarms(static_cast<uint8_t>(tmNow.tm_hour),
                static_cast<uint8_t>(tmNow.tm_min),
                static_cast<uint8_t>(tmNow.tm_wday),
                nowUtc);

    persistSnoozesIfDirty();

    if (!hasActiveAlarms()) {
        // No enabled alarm and no pending snooze: there is nothing a timed
        // wake-up could discover. Let the caller wait for a message instead of
        // waking this service every minute for the rest of the app's life.
        LOG_DEBUG("Nothing armed; no timed wake-up needed\n");
        return kNoWork;
    }

    // Time until the next minute, when the next alarm can become due. Clamped:
    // tm_sec is 0..60 (60 on a leap second), and a 0 ms wait would spin.
    const int32_t secondsLeft = 60 - static_cast<int32_t>(tmNow.tm_sec);
    const uint32_t nextCheckMs = static_cast<uint32_t>(secondsLeft > 0 ? secondsLeft : 1) * 1000u;

    LOG_DEBUG("Next alarm check in %u ms\n", nextCheckMs);
    return nextCheckMs;
}

const std::vector<Alarm>& AlarmManager::getAlarmList()
{
    return mAlarms;
}

bool AlarmManager::saveAlarmList(const std::vector<Alarm>& list)
{
    bool status = saveToFile(list);

    if (status) {
        mAlarms = list;

        // Drop snoozes the edited list no longer justifies
        removeObsoleteSnoozedAlarms();
        persistSnoozesIfDirty();

        LOG_DEBUG("Alarms saved\n");

        if (mObserver) {
            mObserver->onListChanged(mAlarms);
        }
    }

    return status;
}

void AlarmManager::disableAlarm(const Alarm& alarm)
{
    auto it = std::find_if(mSnoozedAlarms.begin(), mSnoozedAlarms.end(),
        [&](const SnoozedAlarm& snoozed) {
            return snoozed.info == alarm;
        });

    if (it != mSnoozedAlarms.end()) {
        LOG_DEBUG("Removing snoozed alarm: %02d:%02d\n",
            it->info.timeHours, it->info.timeMinutes);
        mSnoozedAlarms.erase(it);
        mSnoozesDirty = true;
    }

    removeObsoleteSnoozedAlarms();
    persistSnoozesIfDirty();
}

void AlarmManager::disableAllActiveAlarm()
{
    if (mSnoozedAlarms.empty()) {
        return;
    }

    for (const auto& snoozed : mSnoozedAlarms) {
        LOG_DEBUG("Removing snoozed alarm: %02d:%02d\n",
            snoozed.info.timeHours, snoozed.info.timeMinutes);
        (void)snoozed;
    }

    mSnoozedAlarms.clear();
    mSnoozesDirty = true;
    persistSnoozesIfDirty();
}

void AlarmManager::snoozeAlarm(const Alarm& alarm)
{
    // The alarm was already added to mSnoozedAlarms when it first fired
    // (see checkAlarms -> armSnooze). Re-triggering after kSnoozedTimeMinutes
    // is handled automatically by execute(). Nothing to do here.
    LOG_DEBUG("Snooze acknowledged for alarm %02d:%02d\n",
        alarm.timeHours, alarm.timeMinutes);
    (void)alarm;
}

void AlarmManager::snoozeAllActiveAlarm()
{
    // Same as snoozeAlarm() -- all active alarms are already tracked.
    LOG_DEBUG("Snooze all acknowledged (%u active)\n",
        static_cast<unsigned>(mSnoozedAlarms.size()));
}

bool AlarmManager::hasActiveAlarms() const
{
    bool hasEnabledAlarms = std::any_of(mAlarms.begin(), mAlarms.end(),
        [](const Alarm& alarm) { return alarm.on; });

    return hasEnabledAlarms || !mSnoozedAlarms.empty();
}


// -- Private ------------------------------------------------------------------

bool AlarmManager::saveToFile(const std::vector<Alarm>& alarms)
{
    bool rv = false;
    size_t bw = 0;

    auto file = mKernel.fs.file(skFilePath);
    if (!file) {
        LOG_ERROR("Failed to create file object for %s\n", skFilePath);
        return false;
    }

    size_t len = createJSON(alarms, mBuffer, sizeof(mBuffer));
    if (len > 0) {
        if (file->open(true, true)) {
            if (file->write(mBuffer, len, bw) && bw == len) {
                rv = true;
            }
            file->close();
        }
    }

    if (!rv) {
        LOG_ERROR("Failed to save alarms!\n");
    }
    return rv;
}

bool AlarmManager::loadFromFile(std::vector<Alarm>& alarms)
{
    bool rv = false;
    size_t br = 0;

    auto file = mKernel.fs.file(skFilePath);
    if (!file) {
        LOG_ERROR("Failed to create file object for %s\n", skFilePath);
        return false;
    }

    if (!file->exist()) {
        LOG_INFO("No saved file with alarms %s\n", skFilePath);
        return false;
    }

    size_t len = file->size();
    if (len < sizeof(mBuffer)) {
        if (file->open()) {
            if (file->read(mBuffer, len, br) && br > 0) {
                rv = parseJSON(mBuffer, len, alarms);
            }
            file->close();
        }
    } else {
        LOG_ERROR("Buffer is too small. Required %u bytes\n", len);
    }

    if (!rv) {
        LOG_ERROR("Can't read alarms file or file is corrupted.\n");
    }

    return rv;
}

bool AlarmManager::parseJSON(char* buff, uint32_t length, std::vector<Alarm>& alarms)
{
    SDK::JsonStreamReader reader{ buff, length };
    if (!reader.validate()) {
        LOG_ERROR("JSON is invalid\n");
        return false;
    }

    alarms.clear();

    size_t arrayLength = 0;
    if (!reader.getArrayLength("alarms", arrayLength)) {
        LOG_ERROR("Failed to get alarms array length\n");
        return false;
    }

    alarms.reserve(arrayLength);

    for (size_t i = 0; i < arrayLength; i++) {
        Alarm alarm{};
        char query[32];

        snprintf(query, sizeof(query), "alarms[%u].on", static_cast<unsigned>(i));
        if (!reader.get(query, alarm.on)) {
            LOG_ERROR("Failed to parse 'on' field for alarm %u\n", static_cast<unsigned>(i));
            continue;
        }

        snprintf(query, sizeof(query), "alarms[%u].time_h", static_cast<unsigned>(i));
        if (!reader.get(query, alarm.timeHours) || alarm.timeHours >= 24) {
            LOG_ERROR("Failed to parse or invalid 'time_h' for alarm %u\n", static_cast<unsigned>(i));
            continue;
        }

        snprintf(query, sizeof(query), "alarms[%u].time_m", static_cast<unsigned>(i));
        if (!reader.get(query, alarm.timeMinutes) || alarm.timeMinutes >= 60) {
            LOG_ERROR("Failed to parse or invalid 'time_m' for alarm %u\n", static_cast<unsigned>(i));
            continue;
        }

        snprintf(query, sizeof(query), "alarms[%u].repeat", static_cast<unsigned>(i));
        std::string_view repeatStr;
        if (!reader.get(query, repeatStr)) {
            LOG_ERROR("Failed to parse 'repeat' for alarm %u\n", static_cast<unsigned>(i));
            continue;
        }

        bool repeatFound = false;
        for (uint8_t j = 0; j < Alarm::REPEAT_COUNT; j++) {
            if (kRepeatJsonKeyValue[j] == repeatStr) {
                alarm.repeat = static_cast<Alarm::Repeat>(j);
                repeatFound = true;
                break;
            }
        }
        if (!repeatFound) {
            LOG_ERROR("Invalid 'repeat' value for alarm %u: %.*s\n",
                static_cast<unsigned>(i), static_cast<int>(repeatStr.length()), repeatStr.data());
            continue;
        }

        snprintf(query, sizeof(query), "alarms[%u].effect", static_cast<unsigned>(i));
        std::string_view effectStr;
        if (!reader.get(query, effectStr)) {
            LOG_ERROR("Failed to parse 'effect' for alarm %u\n", static_cast<unsigned>(i));
            continue;
        }

        bool effectFound = false;
        for (uint8_t j = 0; j < Alarm::EFFECT_COUNT; j++) {
            if (kEffectJsonKeyValue[j] == effectStr) {
                alarm.effect = static_cast<Alarm::Effect>(j);
                effectFound = true;
                break;
            }
        }
        if (!effectFound) {
            LOG_ERROR("Invalid 'effect' value for alarm %u: %.*s\n",
                static_cast<unsigned>(i), static_cast<int>(effectStr.length()), effectStr.data());
            continue;
        }

        alarms.push_back(alarm);
    }

    LOG_DEBUG("Parsed %u alarms\n", static_cast<unsigned>(alarms.size()));
    return true;
}

uint32_t AlarmManager::createJSON(const std::vector<Alarm>& alarms, char* buff, uint32_t buffSize)
{
    SDK::JsonStreamWriter writer{ buff, buffSize };

    writer.startMap();

    {
        SDK::JsonStreamWriter::KeyedArrayScope alarmsArray{ writer, "alarms", alarms.size() };

        for (const auto& alarm : alarms) {
            SDK::JsonStreamWriter::MapScope alarmObj{ writer };

            writer.add("on",     alarm.on);
            writer.add("time_h", alarm.timeHours);
            writer.add("time_m", alarm.timeMinutes);

            if (alarm.repeat < Alarm::REPEAT_COUNT) {
                writer.add("repeat", kRepeatJsonKeyValue[alarm.repeat].data());
            } else {
                LOG_ERROR("Invalid repeat value: %u\n", static_cast<unsigned>(alarm.repeat));
                writer.add("repeat", "no");
            }

            if (alarm.effect < Alarm::EFFECT_COUNT) {
                writer.add("effect", kEffectJsonKeyValue[alarm.effect].data());
            } else {
                LOG_ERROR("Invalid effect value: %u\n", static_cast<unsigned>(alarm.effect));
                writer.add("effect", "beep_vibro");
            }
        }
    }

    writer.endMap();

    if (writer.isError()) {
        LOG_ERROR("Failed to create JSON\n");
        buff[0] = '\0';
        return 0;
    }

    uint32_t jsonLength = static_cast<uint32_t>(strlen(buff));
    LOG_DEBUG("Created JSON: %u bytes\n", jsonLength);
    return jsonLength;
}

void AlarmManager::dump(const std::vector<Alarm>& alarms)
{
    for (size_t i = 0; i < alarms.size(); i++) {
        LOG_DEBUG("alarm %u: on=%d %02d:%02d repeat=%d effect=%d\n",
            static_cast<unsigned>(i), alarms[i].on,
            alarms[i].timeHours, alarms[i].timeMinutes,
            alarms[i].repeat, alarms[i].effect);
    }
}

void AlarmManager::checkAlarms(uint8_t currentHour, uint8_t currentMinute,
                                uint8_t currentDay, std::time_t nowUtc)
{
    const int64_t nowMinute = static_cast<int64_t>(nowUtc / 60);
    bool needSave = false;

    forgetStaleFired(nowMinute);

    for (auto& alarm : mAlarms) {
        if (!alarm.on) continue;

        if (alarm.timeHours   != currentHour   ||
            alarm.timeMinutes != currentMinute ||
            !isAlarmDueToday(alarm, currentDay))
        {
            continue;
        }

        // Already rung this minute — possibly stopped since, which is exactly
        // when the old code rang it again — or waiting out a snooze.
        if (hasFiredThisMinute(alarm, nowMinute) || hasPendingSnooze(alarm)) {
            continue;
        }

        LOG_INFO("Triggering alarm: %02d:%02d\n", alarm.timeHours, alarm.timeMinutes);

        if (mObserver) {
            mObserver->onAlarm(alarm);
        }
        markFired(alarm, nowMinute);

        // A one-time alarm is switched off by its own trigger, so its snooze is
        // armed as detached: it no longer depends on the parent being enabled.
        const bool oneShot = (alarm.repeat == Alarm::REPEAT_NO);
        armSnooze(alarm, nowUtc, oneShot);

        if (oneShot) {
            alarm.on = false;
            needSave = true;
            LOG_DEBUG("Disabled one-time alarm: %02d:%02d\n",
                alarm.timeHours, alarm.timeMinutes);
        }
    }

    // Re-trigger snoozed alarms whose deadline has passed. Comparing absolute
    // timestamps rather than hour/minute equality: an entry that misses its
    // minute must still ring, not sit here for ever suppressing its own alarm.
    auto it = mSnoozedAlarms.begin();
    while (it != mSnoozedAlarms.end()) {
        if (it->ringsLeft == 0) {
            // Not reachable through armSnooze(); a hand-edited or truncated
            // snooze file could still produce it.
            it = mSnoozedAlarms.erase(it);
            mSnoozesDirty = true;
            continue;
        }

        const std::time_t due = it->nextTriggerAt;

        if (nowUtc + static_cast<std::time_t>(kSnoozedTimeMinutes) * 60 < due) {
            // The clock stepped backwards past any plausible deadline (a time
            // sync, or a restore from a stale file). Re-anchor so the entry
            // cannot outlive its usefulness.
            LOG_INFO("Clock stepped back; re-anchoring snooze %02d:%02d\n",
                it->info.timeHours, it->info.timeMinutes);
            it->nextTriggerAt = snoozeDeadline(nowUtc);
            mSnoozesDirty = true;
            ++it;
            continue;
        }

        if (nowUtc < due) {
            ++it;
            continue;
        }

        if (nowUtc - due > kSnoozeLateGraceSec) {
            // The watch was off, or the clock jumped forward. Ringing now would
            // ring at the wrong time; drop the entry so nothing stays stuck.
            LOG_INFO("Snooze %02d:%02d missed its slot by %ld s; dropping it\n",
                it->info.timeHours, it->info.timeMinutes,
                static_cast<long>(nowUtc - due));
            it = mSnoozedAlarms.erase(it);
            mSnoozesDirty = true;
            continue;
        }

        --it->ringsLeft;
        mSnoozesDirty = true;

        LOG_INFO("Re-triggering snoozed alarm: %02d:%02d (%u ring(s) left)\n",
            it->info.timeHours, it->info.timeMinutes,
            static_cast<unsigned>(it->ringsLeft));

        if (mObserver) {
            mObserver->onAlarm(it->info);
        }
        markFired(it->info, nowMinute);

        if (it->ringsLeft > 0) {
            it->nextTriggerAt = snoozeDeadline(nowUtc);
            ++it;
        } else {
            LOG_INFO("Snooze exhausted, removing alarm: %02d:%02d\n",
                it->info.timeHours, it->info.timeMinutes);
            it = mSnoozedAlarms.erase(it);
        }
    }

    if (needSave) {
        saveToFile(mAlarms);
        if (mObserver) {
            mObserver->onListChanged(mAlarms);
        }
    }
}

std::time_t AlarmManager::snoozeDeadline(std::time_t nowUtc) const
{
    // Anchored to the start of the current minute, not to the exact second the
    // alarm rang. The service wakes on minute boundaries, so an unaligned
    // deadline a second or two past one of them would not be seen until the
    // following wake -- turning every five-minute snooze into six.
    const std::time_t minuteStart = nowUtc - (nowUtc % 60);
    return minuteStart + static_cast<std::time_t>(kSnoozedTimeMinutes) * 60;
}

void AlarmManager::armSnooze(const Alarm& alarm, std::time_t nowUtc, bool detached)
{
    if (mSnoozedAlarms.size() >= kMaxSnoozes) {
        LOG_ERROR("Tracking %u snoozes already; not arming %02d:%02d\n",
            static_cast<unsigned>(mSnoozedAlarms.size()),
            alarm.timeHours, alarm.timeMinutes);
        return;
    }

    SnoozedAlarm snoozed;
    snoozed.info          = alarm;
    // The GUI reads `on` as "this alarm is ringing" (Model::switchToNext-
    // PriorityScreen), so a re-trigger must carry it set even when the parent
    // one-time alarm has since been switched off.
    snoozed.info.on       = true;
    snoozed.ringsLeft     = kMaxSnoozeRings;
    snoozed.nextTriggerAt = snoozeDeadline(nowUtc);
    snoozed.detached      = detached;

    mSnoozedAlarms.push_back(snoozed);
    mSnoozesDirty = true;

    LOG_DEBUG("Snooze armed for %02d:%02d in %u min\n",
        alarm.timeHours, alarm.timeMinutes,
        static_cast<unsigned>(kSnoozedTimeMinutes));
}

void AlarmManager::removeObsoleteSnoozedAlarms()
{
    const size_t before = mSnoozedAlarms.size();

    mSnoozedAlarms.erase(
        std::remove_if(mSnoozedAlarms.begin(), mSnoozedAlarms.end(),
            [this](const SnoozedAlarm& snoozed) {
                auto it = std::find(mAlarms.begin(), mAlarms.end(), snoozed.info);
                if (it == mAlarms.end()) {
                    return true;    // alarm deleted, or edited to another time
                }
                if (snoozed.detached) {
                    return false;   // its own trigger cleared `on`, not the user
                }
                return !it->on;     // the user switched the alarm off
            }),
        mSnoozedAlarms.end());

    if (mSnoozedAlarms.size() != before) {
        mSnoozesDirty = true;
    }
}

void AlarmManager::markFired(const Alarm& alarm, int64_t nowMinute)
{
    for (auto& fired : mFiredAlarms) {
        if (fired.info == alarm) {
            fired.minute = nowMinute;
            return;
        }
    }

    mFiredAlarms.push_back(FiredAlarm{ alarm, nowMinute });
}

bool AlarmManager::hasFiredThisMinute(const Alarm& alarm, int64_t nowMinute) const
{
    return std::any_of(mFiredAlarms.begin(), mFiredAlarms.end(),
        [&](const FiredAlarm& fired) {
            return fired.minute == nowMinute && fired.info == alarm;
        });
}

void AlarmManager::forgetStaleFired(int64_t nowMinute)
{
    // The latch covers one minute only, so anything not stamped with the
    // current one is spent — including stamps in the future after a clock step.
    mFiredAlarms.erase(
        std::remove_if(mFiredAlarms.begin(), mFiredAlarms.end(),
            [nowMinute](const FiredAlarm& fired) {
                return fired.minute != nowMinute;
            }),
        mFiredAlarms.end());
}

bool AlarmManager::isAlarmDueToday(const Alarm& alarm, uint8_t currentDay) const
{
    switch (alarm.repeat) {
    case Alarm::REPEAT_NO:
    case Alarm::REPEAT_EVERY_DAY:
        return true;
    case Alarm::REPEAT_WEEK_DAYS:
        return currentDay >= 1 && currentDay <= 5;
    case Alarm::REPEAT_WEEKENDS:
        return currentDay == 0 || currentDay == 6;
    case Alarm::REPEAT_MONDAY:    return currentDay == 1;
    case Alarm::REPEAT_TUESDAY:   return currentDay == 2;
    case Alarm::REPEAT_WEDNESDAY: return currentDay == 3;
    case Alarm::REPEAT_THURSDAY:  return currentDay == 4;
    case Alarm::REPEAT_FRIDAY:    return currentDay == 5;
    case Alarm::REPEAT_SATURDAY:  return currentDay == 6;
    case Alarm::REPEAT_SUNDAY:    return currentDay == 0;
    default:                      return false;
    }
}

bool AlarmManager::hasPendingSnooze(const Alarm& alarm) const
{
    return std::any_of(mSnoozedAlarms.begin(), mSnoozedAlarms.end(),
        [&](const SnoozedAlarm& snoozed) {
            return snoozed.info == alarm;
        });
}


// -- Snooze persistence -------------------------------------------------------
//
// Snoozes live in their own file: a reboot, an OTA or a low-battery shutdown
// gives a service no chance to save on the way out, so a pending snooze is
// written whenever it changes. Keeping it out of alarms.json means snooze data
// can never crowd the alarm list out of the shared serialisation buffer, and a
// failure to write one cannot cost the user the other.

void AlarmManager::persistSnoozesIfDirty()
{
    if (!mSnoozesDirty) {
        return;
    }

    // Cleared either way: a storage error is logged, and the next change
    // retries, rather than every execute() re-attempting a failing write.
    mSnoozesDirty = false;
    saveSnoozesToFile();
}

bool AlarmManager::saveSnoozesToFile()
{
    bool rv = false;
    size_t bw = 0;

    auto file = mKernel.fs.file(skSnoozeFilePath);
    if (!file) {
        LOG_ERROR("Failed to create file object for %s\n", skSnoozeFilePath);
        return false;
    }

    size_t len = createSnoozeJSON(mBuffer, sizeof(mBuffer));
    if (len > 0) {
        if (file->open(true, true)) {
            if (file->write(mBuffer, len, bw) && bw == len) {
                rv = true;
            }
            file->close();
        }
    }

    if (!rv) {
        LOG_ERROR("Failed to save snoozes!\n");
    }
    return rv;
}

bool AlarmManager::loadSnoozesFromFile()
{
    bool rv = false;
    size_t br = 0;

    mSnoozedAlarms.clear();

    auto file = mKernel.fs.file(skSnoozeFilePath);
    if (!file) {
        LOG_ERROR("Failed to create file object for %s\n", skSnoozeFilePath);
        return false;
    }

    if (!file->exist()) {
        LOG_INFO("No saved file with snoozes %s\n", skSnoozeFilePath);
        return false;
    }

    size_t len = file->size();
    if (len < sizeof(mBuffer)) {
        if (file->open()) {
            if (file->read(mBuffer, len, br) && br > 0) {
                rv = parseSnoozeJSON(mBuffer, static_cast<uint32_t>(len));
            }
            file->close();
        }
    } else {
        LOG_ERROR("Buffer is too small. Required %u bytes\n", static_cast<unsigned>(len));
    }

    if (!rv) {
        LOG_ERROR("Can't read snoozes file or file is corrupted.\n");
    }

    return rv;
}

uint32_t AlarmManager::createSnoozeJSON(char* buff, uint32_t buffSize)
{
    SDK::JsonStreamWriter writer{ buff, buffSize };

    writer.startMap();

    {
        SDK::JsonStreamWriter::KeyedArrayScope snoozesArray{
            writer, "snoozes", mSnoozedAlarms.size() };

        for (const auto& snoozed : mSnoozedAlarms) {
            SDK::JsonStreamWriter::MapScope snoozeObj{ writer };

            writer.add("time_h", snoozed.info.timeHours);
            writer.add("time_m", snoozed.info.timeMinutes);

            if (snoozed.info.repeat < Alarm::REPEAT_COUNT) {
                writer.add("repeat", kRepeatJsonKeyValue[snoozed.info.repeat].data());
            } else {
                LOG_ERROR("Invalid repeat value: %u\n",
                    static_cast<unsigned>(snoozed.info.repeat));
                writer.add("repeat", "no");
            }

            if (snoozed.info.effect < Alarm::EFFECT_COUNT) {
                writer.add("effect", kEffectJsonKeyValue[snoozed.info.effect].data());
            } else {
                LOG_ERROR("Invalid effect value: %u\n",
                    static_cast<unsigned>(snoozed.info.effect));
                writer.add("effect", "beep_vibro");
            }

            writer.add("rings_left", snoozed.ringsLeft);
            // Epoch seconds as uint32: JsonStreamWriter's 64-bit overloads go
            // through double and print with %g, which rounds a timestamp to six
            // significant digits (a quarter-hour of slop). The 32-bit path is
            // exact, and holds epoch seconds until 2106.
            writer.add("next_at",    static_cast<uint32_t>(snoozed.nextTriggerAt));
            writer.add("detached",   snoozed.detached);
        }
    }

    writer.endMap();

    if (writer.isError()) {
        LOG_ERROR("Failed to create snooze JSON\n");
        buff[0] = '\0';
        return 0;
    }

    uint32_t jsonLength = static_cast<uint32_t>(strlen(buff));
    LOG_DEBUG("Created snooze JSON: %u bytes\n", jsonLength);
    return jsonLength;
}

bool AlarmManager::parseSnoozeJSON(char* buff, uint32_t length)
{
    SDK::JsonStreamReader reader{ buff, length };
    if (!reader.validate()) {
        LOG_ERROR("Snooze JSON is invalid\n");
        return false;
    }

    size_t arrayLength = 0;
    if (!reader.getArrayLength("snoozes", arrayLength)) {
        LOG_ERROR("Failed to get snoozes array length\n");
        return false;
    }

    for (size_t i = 0; i < arrayLength && mSnoozedAlarms.size() < kMaxSnoozes; i++) {
        SnoozedAlarm snoozed{};
        char query[32];

        snprintf(query, sizeof(query), "snoozes[%u].time_h", static_cast<unsigned>(i));
        if (!reader.get(query, snoozed.info.timeHours) || snoozed.info.timeHours >= 24) {
            LOG_ERROR("Failed to parse or invalid 'time_h' for snooze %u\n",
                static_cast<unsigned>(i));
            continue;
        }

        snprintf(query, sizeof(query), "snoozes[%u].time_m", static_cast<unsigned>(i));
        if (!reader.get(query, snoozed.info.timeMinutes) || snoozed.info.timeMinutes >= 60) {
            LOG_ERROR("Failed to parse or invalid 'time_m' for snooze %u\n",
                static_cast<unsigned>(i));
            continue;
        }

        snprintf(query, sizeof(query), "snoozes[%u].repeat", static_cast<unsigned>(i));
        std::string_view repeatStr;
        if (!reader.get(query, repeatStr)) {
            LOG_ERROR("Failed to parse 'repeat' for snooze %u\n", static_cast<unsigned>(i));
            continue;
        }

        bool repeatFound = false;
        for (uint8_t j = 0; j < Alarm::REPEAT_COUNT; j++) {
            if (kRepeatJsonKeyValue[j] == repeatStr) {
                snoozed.info.repeat = static_cast<Alarm::Repeat>(j);
                repeatFound = true;
                break;
            }
        }
        if (!repeatFound) {
            LOG_ERROR("Invalid 'repeat' value for snooze %u\n", static_cast<unsigned>(i));
            continue;
        }

        snprintf(query, sizeof(query), "snoozes[%u].effect", static_cast<unsigned>(i));
        std::string_view effectStr;
        if (!reader.get(query, effectStr)) {
            LOG_ERROR("Failed to parse 'effect' for snooze %u\n", static_cast<unsigned>(i));
            continue;
        }

        bool effectFound = false;
        for (uint8_t j = 0; j < Alarm::EFFECT_COUNT; j++) {
            if (kEffectJsonKeyValue[j] == effectStr) {
                snoozed.info.effect = static_cast<Alarm::Effect>(j);
                effectFound = true;
                break;
            }
        }
        if (!effectFound) {
            LOG_ERROR("Invalid 'effect' value for snooze %u\n", static_cast<unsigned>(i));
            continue;
        }

        snprintf(query, sizeof(query), "snoozes[%u].rings_left", static_cast<unsigned>(i));
        if (!reader.get(query, snoozed.ringsLeft) ||
            snoozed.ringsLeft == 0 || snoozed.ringsLeft > kMaxSnoozeRings) {
            LOG_ERROR("Failed to parse or invalid 'rings_left' for snooze %u\n",
                static_cast<unsigned>(i));
            continue;
        }

        uint32_t nextAt = 0;
        snprintf(query, sizeof(query), "snoozes[%u].next_at", static_cast<unsigned>(i));
        if (!reader.get(query, nextAt) || nextAt == 0) {
            LOG_ERROR("Failed to parse or invalid 'next_at' for snooze %u\n",
                static_cast<unsigned>(i));
            continue;
        }
        snoozed.nextTriggerAt = static_cast<std::time_t>(nextAt);

        snprintf(query, sizeof(query), "snoozes[%u].detached", static_cast<unsigned>(i));
        if (!reader.get(query, snoozed.detached)) {
            LOG_ERROR("Failed to parse 'detached' for snooze %u\n", static_cast<unsigned>(i));
            continue;
        }

        // `on` is the GUI's "ringing" flag and is never persisted; see armSnooze().
        snoozed.info.on = true;

        mSnoozedAlarms.push_back(snoozed);
    }

    LOG_DEBUG("Parsed %u snoozes\n", static_cast<unsigned>(mSnoozedAlarms.size()));
    return true;
}
