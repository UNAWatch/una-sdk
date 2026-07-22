#ifndef MODEL_HPP
#define MODEL_HPP

#include "touchgfx/UIEventListener.hpp"

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Interfaces/ICustomMessageHandler.hpp"
#include <SDK/GUI/Config.hpp>

#include "Commands.hpp"
#include "Stopwatch.hpp"

// ---------------------------------------------------------------------------
// App::Config -- application-level constants (timing, frame rate).
// Screens include this transitively via Presenter -> ModelListener -> Model.hpp.
//
// There is no screen-timeout constant here: unlike most apps this one never
// closes itself. The user leaves by holding R2, and if the kernel suspends the
// GUI the service keeps timing until the app is opened again.
// ---------------------------------------------------------------------------
namespace App::Config
{
constexpr uint32_t kFrameRate = SDK::GUI::Config::kFrameRate;
} // namespace App::Config

class FrontendApplication;
class ModelListener;

/**
 * @class Model
 * @brief GUI-side mirror of the stopwatch that the service owns.
 *
 * The service is the single source of truth, but it is never polled: it sends
 * a snapshot only when something changes, and the running time is worked out
 * here against the same kernel tick the service stamped the snapshot with.
 */
class Model : public touchgfx::UIEventListener,
              public SDK::Interface::IGuiLifeCycleCallback,
              public SDK::Interface::ICustomMessageHandler
{
public:
    Model();

    void bind(ModelListener *listener)
    {
        modelListener = listener;
    }

    FrontendApplication &application();
    void tick();

    /**
     * @brief Exits the application.
     * This method notifies the kernel that the application is exiting and performs
     * any necessary cleanup before termination.
     *
     * Leaving does not lose a running stopwatch: the service owns the state and
     * keeps counting, so re-entering the app picks the time back up.
     */
    void exitApp();

    /** @name Commands to the service
     *
     * Each returns whether the message was accepted for delivery. A command
     * that was sent is always answered with a snapshot, which is what lets the
     * screen trust that a pending change will resolve.
     *  @{ */
    bool startStopwatch() { return mSender.start(); }
    bool pauseStopwatch() { return mSender.pause(); }
    bool lapStopwatch()   { return mSender.lap(); }
    bool resetStopwatch() { return mSender.reset(); }
    /** @} */

    /**
     * @brief Last snapshot received from the service.
     */
    const Stopwatch::State &stopwatch() const { return mState; }

    /**
     * @brief The kernel tick, the same one the service stamps snapshots with.
     */
    uint32_t nowMs() const { return mKernel.sys.getTimeMs(); }

    /**
     * @brief Elapsed time right now, extrapolated from the last snapshot.
     */
    uint32_t elapsedMs() const
    {
        return Stopwatch::elapsed(mState, nowMs());
    }

protected:
    ModelListener* modelListener;           ///< Pointer to model listener

    // Fields required for for GUI <-> Service communication
    const SDK::Kernel& mKernel;             ///< Reference to kernel interface

    bool mInvalidate = false;               ///< Request to redraw current screen

    CustomMessage::Sender mSender;          ///< Outbound commands to the service
    Stopwatch::State      mState;           ///< Last snapshot from the service

    // IGuiLifeCycleCallback
    void onStart()   override;
    void onResume()  override;
    void onSuspend() override;
    void onStop()    override;

    // ICustomMessageHandler
    bool customMessageHandler(SDK::MessageBase *msg) override;

    void setCapabilities();
};

#endif // MODEL_HPP
