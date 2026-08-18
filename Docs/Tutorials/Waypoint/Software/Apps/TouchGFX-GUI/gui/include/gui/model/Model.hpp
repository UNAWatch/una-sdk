#ifndef MODEL_HPP
#define MODEL_HPP

#include "touchgfx/UIEventListener.hpp"

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Interfaces/ICustomMessageHandler.hpp"

#include "gui/common/GuiConfig.hpp"
#include "Commands.hpp"

#include <vector>
#include <memory>

class FrontendApplication;
class ModelListener;

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
     */
    void exitApp();

    /**
     * @brief Asks the service to store the current position as the new target.
     *
     * The service owns the configuration file, so the GUI never touches it
     * directly - it asks, and hears the outcome back on TARGET_SAVED.
     */
    void saveTargetHere();

    /// The last update from the service, so a screen can draw on first show.
    const CustomMessage::NavState &nav() const { return mNav; }

protected:
    ModelListener* modelListener;           ///< Pointer to model listener

    // Fields required for for GUI <-> Service communication
    const SDK::Kernel& mKernel;             ///< Reference to kernel interface

    bool mInvalidate = false;               ///< Request to redraw current screen

    CustomMessage::NavState mNav {};       ///< Latest navigation state

    // IUserApp implementation
    virtual void onStart()   override;
    virtual void onResume()  override;
    virtual void onStop()    override;
    virtual void onSuspend() override;

    // ICustomMessageHandler implementation
    virtual bool customMessageHandler(SDK::MessageBase *msg) override;
};

#endif // MODEL_HPP
