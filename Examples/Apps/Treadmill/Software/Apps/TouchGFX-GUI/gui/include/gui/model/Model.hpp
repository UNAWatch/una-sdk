#ifndef MODEL_HPP
#define MODEL_HPP

#include "touchgfx/UIEventListener.hpp"

#include <texts/TextKeysAndLanguages.hpp>
#include <images/BitmapDatabase.hpp>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Interfaces/ICustomMessageHandler.hpp"
#include "SDK/Calibration/OutdoorStrideCalibConfig.hpp"  // bin layout constants (header-only)
#include <SDK/Utils/Utils.hpp>
#include <SDK/GUI/Config.hpp>
#include <SDK/GUI/Color.hpp>
#include <SDK/GUI/Button.hpp>

#include "Commands.hpp"
#include "Settings.hpp"
#include "ActivitySummary.hpp"
#include "Track.hpp"
#include "AppMenu.hpp"


// ---------------------------------------------------------------------------
// App::Config -- application-level constants (timing, frame rate).
// Screens include this transitively via Presenter -> ModelListener -> Model.hpp.
// ---------------------------------------------------------------------------
namespace App::Config
{
constexpr uint32_t kFrameRate = SDK::GUI::Config::kFrameRate;

constexpr uint32_t kMenuAnimationSteps = 4;
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
constexpr float kMinSpeed = 0.1f;  ///< km/h or mph -- below this show "---" (not moving)
constexpr float kMinHR = 20.0f;    ///< bpm -- below physiological minimum

/// Convert speed in m/s to display units (km/h or mph).
inline float speedToDisplay(float speedMps, bool isImperial)
{
    return isImperial ? speedMps * 2.2369363f : speedMps * 3.6f;
}
} // namespace App::Display


class FrontendApplication;
class ModelListener;

class Model : public touchgfx::UIEventListener,
              public SDK::Interface::IGuiLifeCycleCallback,
              public SDK::Interface::ICustomMessageHandler
{
public:
    Model();

    void bind(ModelListener* listener) { modelListener = listener; }

    // Controls
    FrontendApplication& application();
    App::MenuNav::Nav&   menu();
    void tick();
    void handleKeyEvent(uint8_t key);
    void invalidate();
    void resetIdleTimer();
    void exitApp();

    // Date/Time
    void getDate(uint8_t& month, uint8_t& day, uint8_t& weekday);
    void getTime(uint8_t& h, uint8_t& m, uint8_t& s);

    // Power
    uint8_t getBatteryLevel() const;

    // Latest external-HR link status (SDK::Accessory::State); the kernel only
    // sends on change, so screens read this on activate to show the current icon.
    uint8_t getAccessoryState() const;

    // Settings
    bool isUnitsImperial() const;
    bool is12HourFormat() const;
    const uint8_t* getHrThresholds() const;
    uint8_t        getHrThresholdsCount() const;
    const Settings& getSettings() const;
    void saveSettings(const Settings& sett);

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

    // Hold-to-confirm: which action the shared TrackHoldConfirmation screen performs.
    enum class HoldConfirmMode { Finish, Discard };
    void setHoldConfirmMode(HoldConfirmMode mode);
    HoldConfirmMode getHoldConfirmMode() const;
    /// Post-run Calibrate & Save (§5): send the actual treadmill distance in
    /// metres (<= 0 means "skip" — finalise with the estimate).
    void trackCalibrate(float distanceActualM);
    bool isTrackSummaryAvailable() const;
    const ActivitySummary& getTrackSummary() const;

    // Calibration (Settings -> Calibration). All on-disk access lives in the
    // Service; the GUI only requests a snapshot and issues a clear command.
    /// Snapshot of the calibration state for the View Data screen.
    struct CalibrationView {
        enum Status : uint8_t { UNCALIBRATED = 0, ESTIMATING, CALIBRATED };
        struct Bin {
            uint16_t loSpm = 0;   ///< Bin lower-edge cadence (SPM)
            uint16_t hiSpm = 0;   ///< Bin upper-edge cadence (SPM)
            uint8_t  pct   = 0;   ///< Fill toward validity: steps/kBinValidMinSteps, 0..100
            bool     valid = false;
        };
        Status   status         = UNCALIBRATED;
        uint16_t validBins      = 0;
        float    totalDistanceM = 0.0f;
        uint16_t binCount       = 0;
        Bin      bins[SDK::Calibration::Config::kBinCount] {};
    };

    /// Ask the Service for a fresh calibration snapshot; the reply arrives
    /// asynchronously via ModelListener::onCalibrationData().
    void requestCalibrationData();

    /// Ask the Service to back up + clear the calibration stores.
    void clearCalibrationData();

    /// Last calibration snapshot received from the Service.
    const CalibrationView& getCalibrationView() const { return mCalibView; }

private:
    // Fields required for GUI <-> Service communication
    ModelListener*           modelListener;
    const SDK::Kernel&       mKernel;

    // IGuiLifeCycleCallback
    void onStart()   override;
    void onResume()  override;
    void onSuspend() override;
    void onStop()    override;

    // ICustomMessageHandler
    bool customMessageHandler(SDK::MessageBase* message) override;

    void decIdleTimer();
    bool isAnyKeyPressed(uint8_t key) const;

    // State
    bool     mIsRunning  = false;
    bool     mInvalidate = false;
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
    uint8_t mBatteryLevel   = 0;
    uint8_t mAccessoryState = 0;   // SDK::Accessory::State (0 = UNAVAILABLE)

    // Track
    HoldConfirmMode        mHoldConfirmMode       = HoldConfirmMode::Discard;
    bool                   mPendingIntervalsMode  = false;
    Track::IntervalsData   mPendingAlertIntervals {};  ///< Snapshot from last INTERVALS_PHASE_ALERT
    Track::State           mTrackState            {};
    const ActivitySummary* mActivitySummary = nullptr;
    Track::Data            mTrackData             {};

    // Calibration snapshot (mirrored from the Service on request).
    CalibrationView        mCalibView             {};
};

#endif // MODEL_HPP
