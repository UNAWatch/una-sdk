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
#include <memory>


// ---------------------------------------------------------------------------
// App::Config -- application-level constants (timing, frame rate).
// Screens include this transitively via Presenter -> ModelListener -> Model.hpp.
// ---------------------------------------------------------------------------
namespace App::Config
{
constexpr uint32_t kFrameRate = SDK::GUI::Config::kFrameRate;

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

    // Controls
    FrontendApplication& application();
    void tick();
    void handleKeyEvent(uint8_t key);
    void resetIdleTimer();
    void exitApp();
    void switchToNextPriorityScreen();

    // Timer
    const Timer&        getActiveTimer() const;
    void                playTimer();
    void                stopTimer();
    void                snoozeTimer();
    std::vector<Timer>& getTimerList();
    bool                is12HourFormat() const { return mTimeFormat12h; }
    void                setTimerEditId(size_t id);
    size_t              getTimerEditId();
    void                saveTimer(size_t id, Timer timer);
    void                deleteTimer(size_t id);

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

    // State
    bool     mIsRunning  = false;
    bool     mInvalidate = false;
    uint32_t mIdleTimer  = 0;
    bool     mStayInApp  = false;

    // Timer
    Timer              mActiveTimer  {};
    std::vector<Timer> mTimerList;
    size_t             mEditTimerId  = 0;
    bool               mTimeFormat12h = false;
};

#endif // MODEL_HPP
