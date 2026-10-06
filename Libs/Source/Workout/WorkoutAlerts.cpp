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

}  // namespace SDK::Workout
