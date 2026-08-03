/**
 ******************************************************************************
 * @file    TimerManager.cpp
 * @date    25-07-2026
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Owns the active countdown and the recent-timers store.
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "TimerManager.hpp"

#define LOG_MODULE_PRX      "TimerManager"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/JSON/JsonStreamReader.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

#include <cstdio>
#include <cstring>


TimerManager::TimerManager(const SDK::Kernel& kernel)
    : mKernel(kernel)
{
    mRecents.reserve(kMaxRecents);
}

TimerManager::~TimerManager()
{}


void TimerManager::load()
{
    loadFromFile(mRecents);

    LOG_DEBUG("Recents loaded: %u\n", static_cast<unsigned>(mRecents.size()));

    if (mObserver) {
        mObserver->onRecentsChanged(mRecents);
    }
}


// -- Countdown control --------------------------------------------------------

void TimerManager::arm(uint16_t durationSec, Timer::Effect effect, uint32_t nowMs)
{
    mDurationSec = durationSec;
    mEffect      = effect;
    mRemainingMs = static_cast<uint32_t>(durationSec) * 1000u;
    mEndTick     = nowMs + mRemainingMs;
    mState       = TimerState::RUNNING;
}

void TimerManager::start(uint16_t durationSec, Timer::Effect effect, uint32_t nowMs)
{
    arm(durationSec, effect, nowMs);
    LOG_INFO("Start %u s\n", static_cast<unsigned>(durationSec));
}

void TimerManager::pause(uint32_t nowMs)
{
    if (mState != TimerState::RUNNING) {
        return;
    }

    int32_t left = static_cast<int32_t>(mEndTick - nowMs);
    mRemainingMs = left > 0 ? static_cast<uint32_t>(left) : 0;
    mState       = TimerState::PAUSED;
    LOG_INFO("Pause, %u ms left\n", static_cast<unsigned>(mRemainingMs));
}

void TimerManager::resume(uint32_t nowMs)
{
    if (mState != TimerState::PAUSED) {
        return;
    }

    mEndTick = nowMs + mRemainingMs;
    mState   = TimerState::RUNNING;
    LOG_INFO("Resume\n");
}

void TimerManager::reset(uint32_t nowMs)
{
    // Reset to the full duration, preserving the run state: only a running timer
    // re-arms and keeps counting from the top; any other state (paused, idle, or
    // fired) just restores the remaining time without starting a countdown -- a
    // reset must never silently turn a stopped/fired timer into a running one.
    if (mState == TimerState::RUNNING) {
        arm(mDurationSec, mEffect, nowMs);
    } else {
        mRemainingMs = static_cast<uint32_t>(mDurationSec) * 1000u;
    }
    LOG_INFO("Reset to %u s\n", static_cast<unsigned>(mDurationSec));
}

void TimerManager::stop()
{
    mState       = TimerState::IDLE;
    mRemainingMs = 0;
    LOG_INFO("Stop\n");
}

void TimerManager::repeat(uint32_t /*nowMs*/)
{
    // Re-arm at the full duration but paused, so the user resumes it from the
    // Running screen when ready.
    mRemainingMs = static_cast<uint32_t>(mDurationSec) * 1000u;
    mState       = TimerState::PAUSED;
    LOG_INFO("Repeat %u s\n", static_cast<unsigned>(mDurationSec));
}

uint32_t TimerManager::execute(uint32_t nowMs)
{
    if (mState != TimerState::RUNNING) {
        return kNoTimeout;
    }

    // Wrap-safe: expired once (now - endTick) is non-negative.
    if (static_cast<int32_t>(nowMs - mEndTick) >= 0) {
        mState       = TimerState::FIRED;
        mRemainingMs = 0;

        LOG_INFO("Fired\n");

        if (mObserver) {
            Timer fired{ mDurationSec, mEffect };
            mObserver->onFired(fired);
        }
        return kNoTimeout;
    }

    return mEndTick - nowMs;
}

bool TimerManager::hasActiveTimers() const
{
    return mState == TimerState::RUNNING || mState == TimerState::PAUSED;
}

TimerManager::State TimerManager::getState() const
{
    State s;
    s.state       = mState;
    s.endTick     = mEndTick;
    s.remainingMs = mRemainingMs;
    s.durationSec = mDurationSec;
    s.effect      = mEffect;
    return s;
}


// -- Recents ------------------------------------------------------------------

bool TimerManager::saveRecents(const std::vector<Timer>& list)
{
    std::vector<Timer> capped = list;
    if (capped.size() > kMaxRecents) {
        capped.resize(kMaxRecents);
    }

    bool status = saveToFile(capped);
    if (status) {
        mRecents = std::move(capped);
        LOG_DEBUG("Recents saved: %u\n", static_cast<unsigned>(mRecents.size()));

        if (mObserver) {
            mObserver->onRecentsChanged(mRecents);
        }
    }
    return status;
}


// -- Persistence --------------------------------------------------------------

bool TimerManager::saveToFile(const std::vector<Timer>& list)
{
    bool rv = false;
    size_t bw = 0;

    auto file = mKernel.fs.file(kFilePath);
    if (!file) {
        LOG_ERROR("Failed to create file object for %s\n", kFilePath);
        return false;
    }

    size_t len = createJSON(list, mBuffer, sizeof(mBuffer));
    if (len > 0) {
        if (file->open(true, true)) {
            if (file->write(mBuffer, len, bw) && bw == len) {
                rv = true;
            }
            file->close();
        }
    }

    if (!rv) {
        LOG_ERROR("Failed to save recents!\n");
    }
    return rv;
}

bool TimerManager::loadFromFile(std::vector<Timer>& list)
{
    bool rv = false;
    size_t br = 0;

    auto file = mKernel.fs.file(kFilePath);
    if (!file) {
        LOG_ERROR("Failed to create file object for %s\n", kFilePath);
        return false;
    }

    if (!file->exist()) {
        LOG_INFO("No saved recents file %s\n", kFilePath);
        return false;
    }

    size_t len = file->size();
    if (len < sizeof(mBuffer)) {
        if (file->open()) {
            if (file->read(mBuffer, len, br) && br > 0) {
                rv = parseJSON(mBuffer, len, list);
            }
            file->close();
        }
    } else {
        LOG_ERROR("Buffer is too small. Required %u bytes\n", static_cast<unsigned>(len));
    }

    if (!rv) {
        LOG_ERROR("Can't read recents file or file is corrupted.\n");
    }
    return rv;
}

bool TimerManager::parseJSON(char* buff, uint32_t length, std::vector<Timer>& list)
{
    SDK::JsonStreamReader reader{ buff, length };
    if (!reader.validate()) {
        LOG_ERROR("JSON is invalid\n");
        return false;
    }

    list.clear();

    size_t arrayLength = 0;
    if (!reader.getArrayLength("recents", arrayLength)) {
        LOG_ERROR("Failed to get recents array length\n");
        return false;
    }

    list.reserve(arrayLength);

    for (size_t i = 0; i < arrayLength && list.size() < kMaxRecents; i++) {
        Timer timer{};
        char query[32];

        snprintf(query, sizeof(query), "recents[%u].sec", static_cast<unsigned>(i));
        if (!reader.get(query, timer.durationSec) ||
            timer.durationSec > Timer::kMaxDurationSec) {
            LOG_ERROR("Invalid 'sec' for recent %u\n", static_cast<unsigned>(i));
            continue;
        }

        snprintf(query, sizeof(query), "recents[%u].effect", static_cast<unsigned>(i));
        std::string_view effectStr;
        if (!reader.get(query, effectStr)) {
            LOG_ERROR("Failed to parse 'effect' for recent %u\n", static_cast<unsigned>(i));
            continue;
        }

        bool effectFound = false;
        for (uint8_t j = 0; j < Timer::EFFECT_COUNT; j++) {
            if (kEffectJsonKeyValue[j] == effectStr) {
                timer.effect = static_cast<Timer::Effect>(j);
                effectFound  = true;
                break;
            }
        }
        if (!effectFound) {
            LOG_ERROR("Invalid 'effect' for recent %u\n", static_cast<unsigned>(i));
            continue;
        }

        list.push_back(timer);
    }

    LOG_DEBUG("Parsed %u recents\n", static_cast<unsigned>(list.size()));

    // An empty array is a valid "no recents"; a non-empty array from which every
    // entry was rejected means the file is corrupt -- report it as a failure.
    return arrayLength == 0 || !list.empty();
}

uint32_t TimerManager::createJSON(const std::vector<Timer>& list, char* buff, uint32_t buffSize)
{
    SDK::JsonStreamWriter writer{ buff, buffSize };

    writer.startMap();

    {
        SDK::JsonStreamWriter::KeyedArrayScope recentsArray{ writer, "recents", list.size() };

        for (const auto& timer : list) {
            SDK::JsonStreamWriter::MapScope recentObj{ writer };

            writer.add("sec", timer.durationSec);

            if (timer.effect < Timer::EFFECT_COUNT) {
                writer.add("effect", kEffectJsonKeyValue[timer.effect].data());
            } else {
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
