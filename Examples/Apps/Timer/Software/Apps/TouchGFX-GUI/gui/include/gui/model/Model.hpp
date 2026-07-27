#ifndef MODEL_HPP
#define MODEL_HPP

#include "touchgfx/UIEventListener.hpp"

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Interfaces/ICustomMessageHandler.hpp"
#include <SDK/Utils/Utils.hpp>
#include <SDK/GUI/Config.hpp>
#include <SDK/GUI/Color.hpp>
#include <SDK/GUI/Button.hpp>

#include "Commands.hpp"

#include <vector>


// ---------------------------------------------------------------------------
// App::Config -- application-level constants (timing, frame rate).
// Screens include this transitively via Presenter -> ModelListener -> Model.hpp.
// ---------------------------------------------------------------------------
namespace App::Config
{
constexpr uint32_t kFrameRate = SDK::GUI::Config::kFrameRate;

constexpr uint32_t kMenuAnimationSteps = 4;   ///< Ticks per wheel/scroll step.
constexpr uint32_t kScreenTimeoutSteps = SDK::Utils::secToTicks(30, kFrameRate);  // 30 s
} // namespace App::Config


class FrontendApplication;
class ModelListener;

class Model : public touchgfx::UIEventListener,
              public SDK::Interface::IGuiLifeCycleCallback,
              public SDK::Interface::ICustomMessageHandler
{
public:
    Model();

    void bind(ModelListener* listener) { modelListener = listener; }

    // -- Controls -------------------------------------------------------------

    FrontendApplication& application();
    void tick();
    void handleKeyEvent(uint8_t key);
    void resetIdleTimer();
    void exitApp();

    // -- Countdown ------------------------------------------------------------

    /** @brief Send a start request for the given timer to the service. */
    void startTimer(const Timer& timer);
    void pauseTimer();
    void resumeTimer();
    void resetTimer();
    void stopTimer();
    void repeatTimer();

    TimerState    getState()       const { return mState; }
    uint16_t      getDurationSec() const { return mDurationSec; }
    Timer::Effect getEffect()      const { return mEffect; }

    /** @brief Remaining countdown in milliseconds, extrapolated locally. */
    uint32_t getRemainingMs() const;

    /** @brief True when the last fire happened while the GUI was closed. */
    bool firedFromBackground() const { return mFiredFromBackground; }

    // -- Selection (Main -> Edit -> Alert -> Menu flow) -----------------------

    void         setEditTimer(const Timer& t) { mEditTimer = t; }
    const Timer& getEditTimer() const         { return mEditTimer; }

    // -- Presets & recents ----------------------------------------------------

    /** @brief Fixed preset durations shown at the top of the Main list. */
    const std::vector<Timer>& getPresets() const { return mPresets; }

    const std::vector<Timer>& getRecents() const { return mRecents; }

    /** @brief Add a manually entered timer to recents (dedup, cap) and persist. */
    void addRecent(const Timer& timer);

    /** @brief Remove a timer from recents if present and persist. */
    void removeRecent(const Timer& timer);

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
    bool customMessageHandler(SDK::MessageBase* msg) override;

    void decIdleTimer();
    void setCapabilities();
    bool isAnyKeyPressed(uint8_t key) const;
    void buildPresets();
    bool isPreset(const Timer& timer) const;

    // -- GUI state ------------------------------------------------------------

    bool     mIsRunning  = false;
    bool     mInvalidate = false;
    uint32_t mIdleTimer  = 0;
    bool     mStartupRouted = false;  ///< First service state seen -> initial screen chosen.

    // -- Countdown snapshot (mirrors the service) -----------------------------

    TimerState    mState       = TimerState::IDLE;
    bool          mFiredFromBackground = false;
    uint32_t      mEndTick     = 0;
    uint32_t      mRemainingMs = 0;
    uint16_t      mDurationSec = 0;
    Timer::Effect mEffect      = Timer::EFFECT_BEEP_AND_VIBRO;

    // -- Lists ----------------------------------------------------------------

    Timer              mEditTimer { 60, Timer::EFFECT_BEEP_AND_VIBRO };
    std::vector<Timer> mPresets;
    std::vector<Timer> mRecents;
};

#endif // MODEL_HPP
