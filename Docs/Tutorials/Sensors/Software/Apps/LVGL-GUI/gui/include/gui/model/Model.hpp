/**
 ******************************************************************************
 * @file    Model.hpp
 * @brief   GUI-side state of Sensors and its link to the service.
 *
 * On top of the lifecycle callbacks every GUI model has, this one receives
 * the service's custom messages (Commands.hpp): it registers with
 * SDK::LVGL::Port as the ICustomMessageHandler, decodes each message and
 * forwards the values to the screen through ModelListener.
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

protected:
    ModelListener*     modelListener;   ///< The screen on display
    const SDK::Kernel& mKernel;         ///< The GUI process's kernel interface

    // IGuiLifeCycleCallback
    void onStart() override;
    void onResume() override;
    void onSuspend() override;
    void onStop() override;

    // ICustomMessageHandler: messages from the service
    bool customMessageHandler(SDK::MessageBase* msg) override;
};

#endif // MODEL_HPP
