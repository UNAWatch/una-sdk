#ifndef MODEL_HPP
#define MODEL_HPP

#include "touchgfx/UIEventListener.hpp"

#include <texts/TextKeysAndLanguages.hpp>
#include <images/BitmapDatabase.hpp>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Interfaces/ICustomMessageHandler.hpp"
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
constexpr float kMinDist  = 0.0f;   ///< km or mi  -- negative = no data
constexpr float kMinSpeed = 0.0f;   ///< km/h or mi/h -- negative = no data
constexpr float kMinPace  = 30.0f;  ///< sec/km or sec/mi -- below any human pace
constexpr float kMinHR    = 20.0f;  ///< bpm -- below physiological minimum
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

    // Settings
    bool isUnitsImperial() const;
    const uint8_t* getHrThresholds() const;
    uint8_t        getHrThresholdsCount() const;
    const Settings& getSettings() const;
    void saveSettings(const Settings& sett);

    // GPS
    bool hasGpsFix() const;

    // Track
    void trackStart();
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
    ModelListener*        modelListener;
    const SDK::Kernel&    mKernel;
    CustomMessage::Sender mSrvSender;

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
    bool    mUnitsImperial     = false;
    uint8_t mHrThresholds[App::Config::kHrThresholdsCount] = {};
    uint8_t mHrThresholdsCount = App::Config::kHrThresholdsCount;
    Settings mSettings {};

    // Kernel state
    bool    mGpsFix       = false;
    uint8_t mBatteryLevel = 0;

    // Track
    Track::State           mTrackState      {};
    const ActivitySummary* mActivitySummary = nullptr;
    Track::Data            mTrackData       {};
};

#endif // MODEL_HPP
