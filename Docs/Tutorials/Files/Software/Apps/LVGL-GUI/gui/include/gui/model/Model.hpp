/**
 ******************************************************************************
 * @file    Model.hpp
 * @brief   GUI-side state of Files and its link to the service.
 *
 * The service owns the settings file. The GUI asks for the current values
 * (GET_SETTINGS), receives them (SETTINGS_VALUES) and sends changes back
 * (SET_SETTINGS); see Commands.hpp. The model does the sending and receiving
 * and hands the values to the screen through ModelListener.
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

    /// Ask the service for the current settings; the answer arrives on the listener.
    void requestSettings();

    /// Send new settings to the service, which stores them in its file.
    void updateSettings(float decimalCounter, CustomMessage::ActivityType activityType,
                        CustomMessage::DisplayMode displayMode);

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
