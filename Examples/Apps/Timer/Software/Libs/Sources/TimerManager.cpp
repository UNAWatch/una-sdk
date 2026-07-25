#include "TimerManager.hpp"

#define LOG_MODULE_PRX      "TimerManager"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/JSON/JsonStreamReader.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

#include <algorithm>


TimerManager::TimerManager(const SDK::Kernel& kernel)
    : mKernel(kernel)
{
    mTimers.reserve(kInitialCount);
    mSnoozedTimers.reserve(kInitialCount / 4);
}

TimerManager::~TimerManager()
{}


void TimerManager::load()
{
    loadFromFile(mTimers);

    LOG_DEBUG("Timers loaded\n");
    dump(mTimers);

    if (mObserver) {
        mObserver->onListChanged(mTimers);
    }
}

uint32_t TimerManager::execute(const std::tm& tmNow)
{
    checkTimers(static_cast<uint8_t>(tmNow.tm_hour),
                static_cast<uint8_t>(tmNow.tm_min),
                static_cast<uint8_t>(tmNow.tm_wday),
                tmNow);

    // Calculate time until next minute (when next timers can trigger)
    uint32_t nextCheckMs = (60 - tmNow.tm_sec) * 1000;

    LOG_DEBUG("Next timer check in %u ms\n", nextCheckMs);
    return nextCheckMs;
}

const std::vector<Timer>& TimerManager::getTimerList()
{
    return mTimers;
}

bool TimerManager::saveTimerList(const std::vector<Timer>& list)
{
    bool status = saveToFile(list);

    if (status) {
        mTimers = list;

        // Remove snoozed timers that no longer have a matching enabled timer
        removeObsoleteSnoozedTimers();

        LOG_DEBUG("Timers saved\n");

        if (mObserver) {
            mObserver->onListChanged(mTimers);
        }
    }

    return status;
}

void TimerManager::disableTimer(const Timer& timer)
{
    auto it = std::find_if(mSnoozedTimers.begin(), mSnoozedTimers.end(),
        [&](const SnoozedTimer& snoozed) {
            return snoozed.info == timer;
        });

    if (it != mSnoozedTimers.end()) {
        LOG_DEBUG("Removing snoozed timer: %02d:%02d\n",
            it->info.timeHours, it->info.timeMinutes);
        mSnoozedTimers.erase(it);
    }

    removeObsoleteSnoozedTimers();
}

void TimerManager::disableAllActiveTimer()
{
    if (mSnoozedTimers.empty()) {
        return;
    }

    for (const auto& snoozed : mSnoozedTimers) {
        LOG_DEBUG("Removing snoozed timer: %02d:%02d\n",
            snoozed.info.timeHours, snoozed.info.timeMinutes);
        (void)snoozed;
    }

    mSnoozedTimers.clear();
}

void TimerManager::snoozeTimer(const Timer& timer)
{
    // The timer was already added to mSnoozedTimers when it first fired
    // (see checkTimers -> addSnoozedTimer). Re-triggering after kSnoozedTimeMinutes
    // is handled automatically by execute(). Nothing to do here.
    LOG_DEBUG("Snooze acknowledged for timer %02d:%02d\n",
        timer.timeHours, timer.timeMinutes);
    (void)timer;
}

void TimerManager::snoozeAllActiveTimer()
{
    // Same as snoozeTimer() -- all active timers are already tracked.
    LOG_DEBUG("Snooze all acknowledged (%u active)\n",
        static_cast<unsigned>(mSnoozedTimers.size()));
}

bool TimerManager::hasActiveTimers() const
{
    bool hasEnabledTimers = std::any_of(mTimers.begin(), mTimers.end(),
        [](const Timer& timer) { return timer.on; });

    return hasEnabledTimers || !mSnoozedTimers.empty();
}


// -- Private ------------------------------------------------------------------

bool TimerManager::saveToFile(const std::vector<Timer>& timers)
{
    bool rv = false;
    size_t bw = 0;

    auto file = mKernel.fs.file(skFilePath);
    if (!file) {
        LOG_ERROR("Failed to create file object for %s\n", skFilePath);
        return false;
    }

    size_t len = createJSON(timers, mBuffer, sizeof(mBuffer));
    if (len > 0) {
        if (file->open(true, true)) {
            if (file->write(mBuffer, len, bw) && bw == len) {
                rv = true;
            }
            file->close();
        }
    }

    if (!rv) {
        LOG_ERROR("Failed to save timers!\n");
    }
    return rv;
}

bool TimerManager::loadFromFile(std::vector<Timer>& timers)
{
    bool rv = false;
    size_t br = 0;

    auto file = mKernel.fs.file(skFilePath);
    if (!file) {
        LOG_ERROR("Failed to create file object for %s\n", skFilePath);
        return false;
    }

    if (!file->exist()) {
        LOG_INFO("No saved file with timers %s\n", skFilePath);
        return false;
    }

    size_t len = file->size();
    if (len < sizeof(mBuffer)) {
        if (file->open()) {
            if (file->read(mBuffer, len, br) && br > 0) {
                rv = parseJSON(mBuffer, len, timers);
            }
            file->close();
        }
    } else {
        LOG_ERROR("Buffer is too small. Required %u bytes\n", len);
    }

    if (!rv) {
        LOG_ERROR("Can't read timers file or file is corrupted.\n");
    }

    return rv;
}

bool TimerManager::parseJSON(char* buff, uint32_t length, std::vector<Timer>& timers)
{
    SDK::JsonStreamReader reader{ buff, length };
    if (!reader.validate()) {
        LOG_ERROR("JSON is invalid\n");
        return false;
    }

    timers.clear();

    size_t arrayLength = 0;
    if (!reader.getArrayLength("timers", arrayLength)) {
        LOG_ERROR("Failed to get timers array length\n");
        return false;
    }

    timers.reserve(arrayLength);

    for (size_t i = 0; i < arrayLength; i++) {
        Timer timer{};
        char query[32];

        snprintf(query, sizeof(query), "timers[%u].on", static_cast<unsigned>(i));
        if (!reader.get(query, timer.on)) {
            LOG_ERROR("Failed to parse 'on' field for timer %u\n", static_cast<unsigned>(i));
            continue;
        }

        snprintf(query, sizeof(query), "timers[%u].time_h", static_cast<unsigned>(i));
        if (!reader.get(query, timer.timeHours) || timer.timeHours >= 24) {
            LOG_ERROR("Failed to parse or invalid 'time_h' for timer %u\n", static_cast<unsigned>(i));
            continue;
        }

        snprintf(query, sizeof(query), "timers[%u].time_m", static_cast<unsigned>(i));
        if (!reader.get(query, timer.timeMinutes) || timer.timeMinutes >= 60) {
            LOG_ERROR("Failed to parse or invalid 'time_m' for timer %u\n", static_cast<unsigned>(i));
            continue;
        }

        snprintf(query, sizeof(query), "timers[%u].repeat", static_cast<unsigned>(i));
        std::string_view repeatStr;
        if (!reader.get(query, repeatStr)) {
            LOG_ERROR("Failed to parse 'repeat' for timer %u\n", static_cast<unsigned>(i));
            continue;
        }

        bool repeatFound = false;
        for (uint8_t j = 0; j < Timer::REPEAT_COUNT; j++) {
            if (kRepeatJsonKeyValue[j] == repeatStr) {
                timer.repeat = static_cast<Timer::Repeat>(j);
                repeatFound = true;
                break;
            }
        }
        if (!repeatFound) {
            LOG_ERROR("Invalid 'repeat' value for timer %u: %.*s\n",
                static_cast<unsigned>(i), static_cast<int>(repeatStr.length()), repeatStr.data());
            continue;
        }

        snprintf(query, sizeof(query), "timers[%u].effect", static_cast<unsigned>(i));
        std::string_view effectStr;
        if (!reader.get(query, effectStr)) {
            LOG_ERROR("Failed to parse 'effect' for timer %u\n", static_cast<unsigned>(i));
            continue;
        }

        bool effectFound = false;
        for (uint8_t j = 0; j < Timer::EFFECT_COUNT; j++) {
            if (kEffectJsonKeyValue[j] == effectStr) {
                timer.effect = static_cast<Timer::Effect>(j);
                effectFound = true;
                break;
            }
        }
        if (!effectFound) {
            LOG_ERROR("Invalid 'effect' value for timer %u: %.*s\n",
                static_cast<unsigned>(i), static_cast<int>(effectStr.length()), effectStr.data());
            continue;
        }

        timers.push_back(timer);
    }

    LOG_DEBUG("Parsed %u timers\n", static_cast<unsigned>(timers.size()));
    return true;
}

uint32_t TimerManager::createJSON(const std::vector<Timer>& timers, char* buff, uint32_t buffSize)
{
    SDK::JsonStreamWriter writer{ buff, buffSize };

    writer.startMap();

    {
        SDK::JsonStreamWriter::KeyedArrayScope timersArray{ writer, "timers", timers.size() };

        for (const auto& timer : timers) {
            SDK::JsonStreamWriter::MapScope timerObj{ writer };

            writer.add("on",     timer.on);
            writer.add("time_h", timer.timeHours);
            writer.add("time_m", timer.timeMinutes);

            if (timer.repeat < Timer::REPEAT_COUNT) {
                writer.add("repeat", kRepeatJsonKeyValue[timer.repeat].data());
            } else {
                LOG_ERROR("Invalid repeat value: %u\n", static_cast<unsigned>(timer.repeat));
                writer.add("repeat", "no");
            }

            if (timer.effect < Timer::EFFECT_COUNT) {
                writer.add("effect", kEffectJsonKeyValue[timer.effect].data());
            } else {
                LOG_ERROR("Invalid effect value: %u\n", static_cast<unsigned>(timer.effect));
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

void TimerManager::dump(const std::vector<Timer>& timers)
{
    for (size_t i = 0; i < timers.size(); i++) {
        LOG_DEBUG("timer %u: on=%d %02d:%02d repeat=%d effect=%d\n",
            static_cast<unsigned>(i), timers[i].on,
            timers[i].timeHours, timers[i].timeMinutes,
            timers[i].repeat, timers[i].effect);
    }
}

void TimerManager::checkTimers(uint8_t currentHour, uint8_t currentMinute,
                                uint8_t currentDay, const std::tm& tmNow)
{
    bool needSave = false;

    for (auto& timer : mTimers) {
        if (!timer.on) continue;

        if (timer.timeHours   == currentHour   &&
            timer.timeMinutes == currentMinute  &&
            isTimerDueToday(timer, currentDay)  &&
            !isSnoozed(timer))
        {
            LOG_INFO("Triggering timer: %02d:%02d\n", timer.timeHours, timer.timeMinutes);

            if (mObserver) {
                mObserver->onTimer(timer);
            }
            addSnoozedTimer(timer, tmNow);

            // One-time timers are disabled immediately after first trigger
            if (timer.repeat == Timer::REPEAT_NO) {
                timer.on = false;
                needSave = true;
                LOG_DEBUG("Disabled one-time timer: %02d:%02d\n",
                    timer.timeHours, timer.timeMinutes);
            }
        }
    }

    // Re-trigger snoozed timers whose next interval has elapsed
    auto it = mSnoozedTimers.begin();
    while (it != mSnoozedTimers.end()) {
        if (it->snoozeCount > 0          &&
            it->nextTriggerHour   == currentHour &&
            it->nextTriggerMinute == currentMinute)
        {
            it->snoozeCount--;

            if (it->snoozeCount > 0) {
                LOG_INFO("Re-triggering snoozed timer: %02d:%02d\n",
                    it->info.timeHours, it->info.timeMinutes);
                if (mObserver) {
                    mObserver->onTimer(it->info);
                }
                updateSnoozedTriggerTime(*it, tmNow);
                ++it;
            } else {
                LOG_INFO("Snooze exhausted, removing timer: %02d:%02d\n",
                    it->info.timeHours, it->info.timeMinutes);
                it = mSnoozedTimers.erase(it);
            }
        } else {
            ++it;
        }
    }

    if (needSave) {
        saveToFile(mTimers);
        if (mObserver) {
            mObserver->onListChanged(mTimers);
        }
    }
}

void TimerManager::addSnoozedTimer(const Timer& timer, const std::tm& tmNow)
{
    SnoozedTimer snoozed;
    snoozed.info = timer;
    updateSnoozedTriggerTime(snoozed, tmNow);

    mSnoozedTimers.push_back(std::move(snoozed));
}

void TimerManager::updateSnoozedTriggerTime(SnoozedTimer& snoozed, const std::tm& tmNow)
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

void TimerManager::removeObsoleteSnoozedTimers()
{
    mSnoozedTimers.erase(
        std::remove_if(mSnoozedTimers.begin(), mSnoozedTimers.end(),
            [this](const SnoozedTimer& snoozed) {
                auto it = std::find(mTimers.begin(), mTimers.end(), snoozed.info);
                return it == mTimers.end() || !it->on;
            }),
        mSnoozedTimers.end());
}

bool TimerManager::isTimerDueToday(const Timer& timer, uint8_t currentDay) const
{
    switch (timer.repeat) {
    case Timer::REPEAT_NO:
    case Timer::REPEAT_EVERY_DAY:
        return true;
    case Timer::REPEAT_WEEK_DAYS:
        return currentDay >= 1 && currentDay <= 5;
    case Timer::REPEAT_WEEKENDS:
        return currentDay == 0 || currentDay == 6;
    case Timer::REPEAT_MONDAY:    return currentDay == 1;
    case Timer::REPEAT_TUESDAY:   return currentDay == 2;
    case Timer::REPEAT_WEDNESDAY: return currentDay == 3;
    case Timer::REPEAT_THURSDAY:  return currentDay == 4;
    case Timer::REPEAT_FRIDAY:    return currentDay == 5;
    case Timer::REPEAT_SATURDAY:  return currentDay == 6;
    case Timer::REPEAT_SUNDAY:    return currentDay == 0;
    default:                      return false;
    }
}

bool TimerManager::isSnoozed(const Timer& timer) const
{
    return std::any_of(mSnoozedTimers.begin(), mSnoozedTimers.end(),
        [&](const SnoozedTimer& snoozed) {
            return snoozed.info == timer;
        });
}
