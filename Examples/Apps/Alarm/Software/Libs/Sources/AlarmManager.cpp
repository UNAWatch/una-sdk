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
    mSnoozedAlarms.reserve(kInitialCount / 4);
}

AlarmManager::~AlarmManager()
{}


void AlarmManager::load()
{
    loadFromFile(mAlarms);

    LOG_DEBUG("Alarms loaded\n");
    dump(mAlarms);

    if (mObserver) {
        mObserver->onListChanged(mAlarms);
    }
}

uint32_t AlarmManager::execute(const std::tm& tmNow)
{
    checkAlarms(static_cast<uint8_t>(tmNow.tm_hour),
                static_cast<uint8_t>(tmNow.tm_min),
                static_cast<uint8_t>(tmNow.tm_wday),
                tmNow);

    // Calculate time until next minute (when next alarms can trigger)
    uint32_t nextCheckMs = (60 - tmNow.tm_sec) * 1000;

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

        // Remove snoozed alarms that no longer have a matching enabled alarm
        removeObsoleteSnoozedAlarms();

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
    }

    removeObsoleteSnoozedAlarms();
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
}

void AlarmManager::snoozeAlarm(const Alarm& alarm)
{
    // The alarm was already added to mSnoozedAlarms when it first fired
    // (see checkAlarms -> addSnoozedAlarm). Re-triggering after kSnoozedTimeMinutes
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
                                uint8_t currentDay, const std::tm& tmNow)
{
    bool needSave = false;

    for (auto& alarm : mAlarms) {
        if (!alarm.on) continue;

        if (alarm.timeHours   == currentHour   &&
            alarm.timeMinutes == currentMinute  &&
            isAlarmDueToday(alarm, currentDay)  &&
            !isSnoozed(alarm))
        {
            LOG_INFO("Triggering alarm: %02d:%02d\n", alarm.timeHours, alarm.timeMinutes);

            if (mObserver) {
                mObserver->onAlarm(alarm);
            }
            addSnoozedAlarm(alarm, tmNow);

            // One-time alarms are disabled immediately after first trigger
            if (alarm.repeat == Alarm::REPEAT_NO) {
                alarm.on = false;
                needSave = true;
                LOG_DEBUG("Disabled one-time alarm: %02d:%02d\n",
                    alarm.timeHours, alarm.timeMinutes);
            }
        }
    }

    // Re-trigger snoozed alarms whose next interval has elapsed
    auto it = mSnoozedAlarms.begin();
    while (it != mSnoozedAlarms.end()) {
        if (it->snoozeCount > 0          &&
            it->nextTriggerHour   == currentHour &&
            it->nextTriggerMinute == currentMinute)
        {
            it->snoozeCount--;

            if (it->snoozeCount > 0) {
                LOG_INFO("Re-triggering snoozed alarm: %02d:%02d\n",
                    it->info.timeHours, it->info.timeMinutes);
                if (mObserver) {
                    mObserver->onAlarm(it->info);
                }
                updateSnoozedTriggerTime(*it, tmNow);
                ++it;
            } else {
                LOG_INFO("Snooze exhausted, removing alarm: %02d:%02d\n",
                    it->info.timeHours, it->info.timeMinutes);
                it = mSnoozedAlarms.erase(it);
            }
        } else {
            ++it;
        }
    }

    if (needSave) {
        saveToFile(mAlarms);
        if (mObserver) {
            mObserver->onListChanged(mAlarms);
        }
    }
}

void AlarmManager::addSnoozedAlarm(const Alarm& alarm, const std::tm& tmNow)
{
    SnoozedAlarm snoozed;
    snoozed.info = alarm;
    updateSnoozedTriggerTime(snoozed, tmNow);

    mSnoozedAlarms.push_back(std::move(snoozed));
}

void AlarmManager::updateSnoozedTriggerTime(SnoozedAlarm& snoozed, const std::tm& tmNow)
{
    uint16_t totalMinutes = static_cast<uint16_t>(
        tmNow.tm_hour * 60 + tmNow.tm_min + kSnoozedTimeMinutes);

    // Wrap around midnight
    if (totalMinutes >= 24 * 60) {
        totalMinutes -= 24 * 60;
    }

    snoozed.nextTriggerHour   = static_cast<uint8_t>(totalMinutes / 60);
    snoozed.nextTriggerMinute = static_cast<uint8_t>(totalMinutes % 60);

    LOG_DEBUG("Next snooze trigger: %02d:%02d\n",
        snoozed.nextTriggerHour, snoozed.nextTriggerMinute);
}

void AlarmManager::removeObsoleteSnoozedAlarms()
{
    mSnoozedAlarms.erase(
        std::remove_if(mSnoozedAlarms.begin(), mSnoozedAlarms.end(),
            [this](const SnoozedAlarm& snoozed) {
                auto it = std::find(mAlarms.begin(), mAlarms.end(), snoozed.info);
                return it == mAlarms.end() || !it->on;
            }),
        mSnoozedAlarms.end());
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

bool AlarmManager::isSnoozed(const Alarm& alarm) const
{
    return std::any_of(mSnoozedAlarms.begin(), mSnoozedAlarms.end(),
        [&](const SnoozedAlarm& snoozed) {
            return snoozed.info == alarm;
        });
}
