/**
 ******************************************************************************
 * @file    Model.hpp
 * @brief   GUI-side state of Waypoint and its link to the service.
 *
 * The service owns the configuration and the GPS; it sends a NavState on
 * every fix (Commands.hpp). The model keeps the last one so a screen can
 * draw on first show, and forwards each new one to the listener. Saving the
 * current position is a request to the service, answered with TARGET_SAVED.
 ******************************************************************************
 */

#ifndef MODEL_HPP
#define MODEL_HPP

#include "SDK/Interfaces/ICustomMessageHandler.hpp"
#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Kernel/Kernel.hpp"

#include "Commands.hpp"

class ModelListener;

class Model : public SDK::Interface::IGuiLifeCycleCallback,
              public SDK::Interface::ICustomMessageHandler
{
public:
    Model();

    /// The screen that receives model events. Exactly one is bound at a time.
    void bind(ModelListener* listener) { modelListener = listener; }

    /// Exit the application: the kernel stops both processes.
    void exitApp();

    /// Ask the service to store the current position as the new target.
    void saveTargetHere();

    /// The last update from the service, so a screen can draw on first show.
    const CustomMessage::NavState& nav() const { return mNav; }

protected:
    ModelListener*     modelListener;   ///< The screen on display
    const SDK::Kernel& mKernel;         ///< The GUI process's kernel interface

    CustomMessage::NavState mNav {};    ///< Latest navigation state

    // IGuiLifeCycleCallback
    void onStart() override;
    void onResume() override;
    void onSuspend() override;
    void onStop() override;

    // ICustomMessageHandler: messages from the service
    bool customMessageHandler(SDK::MessageBase* msg) override;
};

#endif // MODEL_HPP
