/**
 ******************************************************************************
 * @file    Model.hpp
 * @brief   GUI-side state of Buttons and its link to the kernel.
 *
 * Unchanged from the HelloWorld tutorial: the model receives the kernel's
 * lifecycle callbacks through SDK::LVGL::Port and exits the app on request.
 ******************************************************************************
 */

#ifndef MODEL_HPP
#define MODEL_HPP

#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Kernel/Kernel.hpp"

class ModelListener;

class Model : public SDK::Interface::IGuiLifeCycleCallback
{
public:
    Model();

    /// The screen that receives model events. Exactly one is bound at a time.
    void bind(ModelListener* listener) { modelListener = listener; }

    /**
     * @brief Exit the application.
     *
     * Tells the kernel the app is done; the kernel stops both processes.
     */
    void exitApp();

protected:
    ModelListener*     modelListener;   ///< The screen on display
    const SDK::Kernel& mKernel;         ///< The GUI process's kernel interface

    // IGuiLifeCycleCallback: the kernel's lifecycle, forwarded by the port.
    void onStart() override;
    void onResume() override;
    void onSuspend() override;
    void onStop() override;
};

#endif // MODEL_HPP
