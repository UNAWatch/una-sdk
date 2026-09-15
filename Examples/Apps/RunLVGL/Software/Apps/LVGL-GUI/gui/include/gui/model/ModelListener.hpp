/**
 ******************************************************************************
 * @file    ModelListener.hpp
 * @brief   Events the Model raises towards the active screen.
 *
 * Same contract as the TouchGFX Run app's ModelListener, without the TouchGFX
 * headers: the Model binds exactly one listener (the screen on display) and
 * calls these as service messages arrive. onSuspend() is the one addition:
 * a screen with a hold or countdown in progress cancels it there, since no
 * button release can reach it while the GUI is off screen.
 ******************************************************************************
 */

#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <cstdint>

#include "Settings.hpp"
#include "Track.hpp"
#include "ActivitySummary.hpp"

class Model;

class ModelListener
{
public:
    ModelListener() : model(nullptr) {}
    virtual ~ModelListener() = default;

    void bind(Model* m) { model = m; }

    virtual void onIdleTimeout() {}
    virtual void onSuspend() {}
    virtual void onGpsFix(bool acquired) { (void)acquired; }
    virtual void onBatteryLevel(uint8_t level) { (void)level; }
    virtual void onDate(uint16_t year, uint8_t month, uint8_t day, uint8_t wday)
    {
        (void)year; (void)month; (void)day; (void)wday;
    }
    virtual void onTime(uint8_t hour, uint8_t minute, uint8_t sec)
    {
        (void)hour; (void)minute; (void)sec;
    }
    virtual void onSettings(const Settings& settings) { (void)settings; }
    virtual void onTrackState(const Track::State& state) { (void)state; }
    virtual void onTrackData(const Track::Data& data) { (void)data; }
    virtual void onLapChanged(uint8_t lapEnd) { (void)lapEnd; }
    virtual void onIntervalsPhaseAlert() {}
    virtual void onIntervalsWorkoutCompleted() {}
    virtual void onActivitySummary(const ActivitySummary& summary) { (void)summary; }
    virtual void onAccessoryStatus(uint8_t state, const char* name) { (void)state; (void)name; }

protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
