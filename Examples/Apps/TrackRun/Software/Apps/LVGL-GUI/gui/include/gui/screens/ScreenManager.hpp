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
    // Structured workouts
    WorkoutToday,
    WorkoutList,
    WorkoutMenu,
    WorkoutDetails,
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
    // Settings > Run Alerts
    MenuRunAlerts,
    MenuRunWalk,
    MenuRunPace,
    MenuRunHr,
    MenuRunPaceLimit,
    MenuRunAlertPicker,
    MenuRunHrList,
    MenuRunAlertSaved,
    // Settings > Tracks
    MenuTracks,
    MenuTrackDetail,
    // The lane picker
    Lane,
    // Activity
    TrackStartConfirm,
    TrackIntervalsCountdown,
    Track,
    TrackIntervalsAlert,
    TrackIntervalsCompleted,
    TrackWorkoutStep,
    TrackAction,
    TrackHoldConfirm,
    TrackLap,
    TrackRunAlert,
    TrackSaved,
    TrackDiscarded,
    TrackSummary,
    TrackNotice,
};

class ScreenManager
{
public:
    static ScreenManager& instance();

    /// Bind the model and show the first screen.
    void start(Model& model, ScreenId first);

    /// Request a switch; performed on the next frame. The Track screen is
    /// reached through the step card or a pop-up while one waits to be shown
    /// (goToWaitingPopup()). False if LVGL had no room to schedule it; the
    /// next goTo() tries again.
    bool goTo(ScreenId id);

    /// Open the step card if a step started while another screen showed, else
    /// the first pop-up waiting (Model::waitingPopup()). False if none is, or
    /// if the switch could not be scheduled.
    bool goToWaitingPopup();

private:
    ScreenManager() = default;

    static void asyncCb(void* user);
    void switchNow(ScreenId id);
    Screen* create(ScreenId id);
    /// The step card or the first pop-up waiting, else Track.
    ScreenId trackOrPopup() const;

    Model*   mModel   = nullptr;
    Screen*  mCurrent = nullptr;
    ScreenId mPending = ScreenId::Main;
    bool     mPendingSet = false;
};

#endif // SCREEN_MANAGER_HPP
