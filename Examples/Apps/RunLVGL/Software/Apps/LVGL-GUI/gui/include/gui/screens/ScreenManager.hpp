/**
 ******************************************************************************
 * @file    ScreenManager.hpp
 * @brief   Owns the active screen and switches between them.
 *
 * The LVGL counterpart of the TouchGFX FrontendApplication's goto*Screen()
 * calls. A switch is requested with goTo() and performed on the next
 * lv_timer_handler() pass (via lv_async_call), never inside the event or
 * message callback that asked for it, so the requesting screen is not deleted
 * under its own feet. Screens are built on entry and destroyed on exit, as
 * TouchGFX views are.
 ******************************************************************************
 */

#ifndef SCREEN_MANAGER_HPP
#define SCREEN_MANAGER_HPP

#include <cstdint>

class Model;
class Screen;

enum class ScreenId : uint8_t {
    Main,
    // Intervals configuration
    MenuIntervals,
    MenuIntervalsRepeats,
    MenuIntervalsRun,
    MenuIntervalsRest,
    MenuIntervalsRunTime,
    MenuIntervalsRunDistance,
    MenuIntervalsRestTime,
    MenuIntervalsRestDistance,
    // Settings
    MenuSettings,
    MenuAlerts,
    MenuAlertDistance,
    MenuAlertTime,
    MenuAlertDistanceSaved,
    MenuAlertTimeSaved,
    // Activity
    TrackStartConfirm,
    TrackIntervalsCountdown,
    Track,
    TrackIntervalsAlert,
    TrackIntervalsCompleted,
    TrackAction,
    TrackHoldConfirm,
    TrackLap,
    TrackSaved,
    TrackDiscarded,
    TrackSummary,
};

class ScreenManager
{
public:
    static ScreenManager& instance();

    /// Bind the model and show the first screen.
    void start(Model& model, ScreenId first);

    /// Request a switch; performed on the next frame.
    void goTo(ScreenId id);

private:
    ScreenManager() = default;

    static void asyncCb(void* user);
    void switchNow(ScreenId id);
    Screen* create(ScreenId id);

    Model*   mModel   = nullptr;
    Screen*  mCurrent = nullptr;
    ScreenId mPending = ScreenId::Main;
    bool     mPendingSet = false;
};

#endif // SCREEN_MANAGER_HPP
