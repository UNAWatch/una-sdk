/**
 ******************************************************************************
 * @file    Model.hpp
 * @brief   GUI-side state of the Run app and its link to the service process.
 *
 * Ported from the TouchGFX Run app's Model. The service <-> GUI contract
 * (Commands.hpp) is unchanged; what differs is how the model plugs into the
 * toolkit: it registers with SDK::LVGL::Port for lifecycle and custom
 * messages, and screens are plain ModelListeners that own an LVGL screen.
 ******************************************************************************
 */

#ifndef MODEL_HPP
#define MODEL_HPP

#include <cstdint>
#include <ctime>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Interfaces/ICustomMessageHandler.hpp"
#include "SDK/Utils/Utils.hpp"
#include "SDK/GUI/Config.hpp"
#include "SDK/GUI/Button.hpp"

#include "Commands.hpp"
#include "Settings.hpp"
#include "ActivitySummary.hpp"
#include "Track.hpp"
#include "AppMenu.hpp"

// ---------------------------------------------------------------------------
// App::Config -- application-level constants (timing, frame rate).
// ---------------------------------------------------------------------------
namespace App::Config
{
constexpr uint32_t kFrameRate = SDK::GUI::Config::kFrameRate;

constexpr uint32_t kMenuAnimationMs    = 400;                                    // wheel slide
constexpr uint32_t kScreenTimeoutSteps = SDK::Utils::secToTicks(30, kFrameRate);  // 30 s

// HR thresholds
constexpr uint8_t kHrThresholdsCount = CustomMessage::kHrThresholdsCount;
} // namespace App::Config

// ---------------------------------------------------------------------------
// App::Display -- minimum valid values for on-screen display.
// Below these thresholds the widget shows "---" instead of a number.
// ---------------------------------------------------------------------------
namespace App::Display
{
constexpr float kMinDist = 0.0f;   ///< km or mi  -- negative = no data
constexpr float kMinPace = 30.0f;  ///< sec/km or sec/mi -- below any human running pace
constexpr float kMinHR = 20.0f;    ///< bpm -- below physiological minimum
} // namespace App::Display


class ModelListener;

class Model : public SDK::Interface::IGuiLifeCycleCallback,
              public SDK::Interface::ICustomMessageHandler
{
public:
    Model();

    /// The screen that receives model events. Exactly one is bound at a time.
    void bind(ModelListener* listener) { modelListener = listener; }

    /// Remembered menu / face positions, so a screen reopens where it was left.
    App::MenuNav::Nav& menu() { return mMenu; }

    // Controls
    void handleKeyEvent(uint8_t key);
    void resetIdleTimer();
    void exitApp();

    // Date/Time
    void getDate(uint8_t& month, uint8_t& day, uint8_t& weekday) const;
    void getTime(uint8_t& h, uint8_t& m, uint8_t& s) const;
    /// Today's local date as YYYYMMDD, as workout dates are; 0 before the
    /// clock is known.
    uint32_t getTodayYmd() const;

    // Power
    uint8_t getBatteryLevel() const;

    // Settings
    bool isUnitsImperial() const;
    bool is12HourFormat() const;
    const uint8_t* getHrThresholds() const;
    uint8_t        getHrThresholdsCount() const;
    const Settings& getSettings() const;
    void saveSettings(const Settings& sett);

    // GPS
    bool hasGpsFix() const;

    // Latest external-HR link status (SDK::Accessory::State); the kernel only
    // sends on change, so screens read this on activate to show the current icon.
    uint8_t getAccessoryState() const;

    // Hold-to-confirm: which action the shared TrackHoldConfirmation screen performs.
    enum class HoldConfirmMode { Finish, Discard, EndWorkout };
    void setHoldConfirmMode(HoldConfirmMode mode);
    HoldConfirmMode getHoldConfirmMode() const;

    // Track
    void setPendingIntervalsMode(bool mode);
    bool isPendingIntervalsMode() const;
    const Track::IntervalsData& getPendingAlertIntervals() const;
    void trackStart(bool intervalsMode);
    void intervalsNextPhase();
    bool isTrackActive() const;
    void trackPause();
    void trackResume();
    bool isTrackPaused() const;
    const Track::Data& getTrackData() const;
    void saveLap();
    void saveTrack();
    void discardTrack();
    bool isTrackSummaryAvailable() const;
    const ActivitySummary& getTrackSummary() const;

    // Structured workouts
    static constexpr int16_t kNoWorkout = CustomMessage::TrackStart::kNoWorkout;

    /// What the workout faces show, from the latest WORKOUT_DATA.
    struct WorkoutFace {
        uint32_t remainingMs = 0;
        uint32_t remainingCm = 0;
        uint32_t stepTimeMs  = 0;
        uint32_t rep         = 0;
        uint32_t reps        = 0;
        float    value       = 0.0f;  ///< m/s or bpm; 0 with no target
        float    arc         = 0.0f;  ///< 0-1 along the gauge
        uint8_t  stepEnd     = 0;     ///< SDK::Workout::StepEnd
        uint8_t  target      = 0;     ///< SDK::Workout::Target
        uint8_t  zone        = 0;     ///< SDK::Workout::Zone
        uint8_t  intensity   = 0;     ///< SDK::Workout::Intensity
        bool     leadIn      = false;
    };

    /// A step for the next-step card and faces, from WORKOUT_STEP.
    struct WorkoutStepInfo {
        char     notes[SDK::Workout::kStepNotesBytes] = {};
        char     duration[24] = {};
        char     target[32]   = {};
        uint32_t rep       = 0;
        uint32_t reps      = 0;
        uint8_t  intensity = 0;
        bool     none      = true;  ///< No step (next: nothing follows)
    };

    /// The workout list from the service; null until it arrives. The service
    /// rebuilds it only while no run is going, and sends WORKOUT_LIST again
    /// when it does.
    const SDK::Workout::Library* getWorkoutLibrary() const;
    /// True once, if today's scheduled workout should be offered at launch.
    bool takeTodayPrompt();
    uint16_t getTodayWorkout() const;

    /// The workout the list or the today prompt is showing.
    void     setChosenWorkout(uint16_t index);
    uint16_t getChosenWorkout() const;

    /// Where Details goes back to: the today prompt, or the workout's menu.
    void setDetailsFromToday(bool today) { mDetailsFromToday = today; }
    bool isDetailsFromToday() const { return mDetailsFromToday; }

    /// The workout the next run follows; kNoWorkout for none.
    void    armWorkout(int16_t index);
    int16_t getArmedWorkout() const;

    /// Ask for a workout in full; onWorkoutDetails() follows.
    void requestWorkoutDetails(uint16_t index);
    /// The workout asked for, or null if it could not be read. Valid until
    /// the next request or run.
    const SDK::Workout::Program* getWorkoutDetails() const;

    void endWorkout();
    const WorkoutFace&     getWorkoutFace() const;
    const WorkoutStepInfo& getWorkoutStep(bool next) const;
    /// True once after a step starts, for the next-step card.
    bool takeStepCard();
    /// A step card is waiting to be shown; unlike takeStepCard() it stays.
    bool hasStepCard() const { return mStepCard; }

private:
    // Fields required for GUI <-> Service communication
    ModelListener*           modelListener;
    const SDK::Kernel&       mKernel;

    // IGuiLifeCycleCallback
    void onStart()   override;
    void onFrame()   override;
    void onResume()  override;
    void onSuspend() override;
    void onStop()    override;

    // ICustomMessageHandler
    bool customMessageHandler(SDK::MessageBase* message) override;

    void decIdleTimer();
    static bool isClick(uint8_t key);

    // State
    bool     mIsRunning  = false;
    uint32_t mIdleTimer  = 0;

    App::MenuNav::Nav mMenu {};
    std::tm           mTime {};

    // Settings (mirrored from Service)
    bool mUnitsImperial = false;
    bool mTimeFormat12h = false;
    uint8_t mHrThresholds[App::Config::kHrThresholdsCount] = {};
    uint8_t mHrThresholdsCount = App::Config::kHrThresholdsCount;
    Settings mSettings {};

    // Kernel state
    bool    mGpsFix         = false;
    uint8_t mBatteryLevel   = 0;
    uint8_t mAccessoryState = 0;   // SDK::Accessory::State (0 = UNAVAILABLE)

    // Track
    HoldConfirmMode        mHoldConfirmMode       = HoldConfirmMode::Discard;
    bool                   mPendingIntervalsMode  = false;
    Track::IntervalsData   mPendingAlertIntervals {};  ///< Snapshot from last INTERVALS_PHASE_ALERT
    Track::State           mTrackState            {};
    const ActivitySummary* mActivitySummary = nullptr;
    Track::Data            mTrackData             {};

    // Structured workouts
    const SDK::Workout::Library* mWorkoutLibrary = nullptr;
    const SDK::Workout::Program* mWorkoutDetails = nullptr;
    uint16_t        mTodayWorkout   = CustomMessage::WorkoutList::kNoToday;
    bool            mTodayPrompted  = false;
    uint16_t        mChosenWorkout  = 0;
    int16_t         mArmedWorkout   = kNoWorkout;
    bool            mStepCard       = false;
    bool            mDetailsFromToday = false;
    WorkoutFace     mWorkoutFace    {};
    WorkoutStepInfo mWorkoutStep    {};
    WorkoutStepInfo mWorkoutNext    {};
};

#endif // MODEL_HPP
