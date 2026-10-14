/**
 ******************************************************************************
 * @file    WorkoutAlerts.cpp
 * @brief   When to alert while a workout runs.
 ******************************************************************************
 */

#include "SDK/Workout/WorkoutAlerts.hpp"

namespace SDK::Workout {

void LeadInAlert::stepStarted()
{
    mStarted = false;
    mBeeps   = 0;
}

bool LeadInAlert::tick(const Engine::Status& status)
{
    if (!status.running) {
        return false;
    }
    if (status.leadIn) {
        mStarted = true;
    }
    if (!mStarted || mBeeps >= kMaxBeeps) {
        return false;
    }
    ++mBeeps;
    return true;
}

void ZoneAlerts::stepStarted()
{
    mStepSec    = 0;
    mSinceAlert = 0;
    mAlerted    = Zone::Unknown;
}

ZoneAlerts::Alert ZoneAlerts::tick(Zone zone)
{
    ++mStepSec;
    if (mAlerted != Zone::Unknown) {
        ++mSinceAlert;
    }
    switch (zone) {
    case Zone::In:
        if (mAlerted != Zone::Unknown) {
            mAlerted = Zone::Unknown;
            return Alert::BackIn;
        }
        return Alert::None;
    case Zone::Above:
    case Zone::Below:
        if (mStepSec <= kQuietStartSec) {
            return Alert::None;
        }
        if (zone != mAlerted || mSinceAlert >= kRepeatSec) {
            mAlerted    = zone;
            mSinceAlert = 0;
            return zone == Zone::Above ? Alert::Above : Alert::Below;
        }
        return Alert::None;
    default:
        return Alert::None;
    }
}

}  // namespace SDK::Workout
