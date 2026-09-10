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
    enum class HoldConfirmMode { Finish, Discard };
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
};

#endif // MODEL_HPP
