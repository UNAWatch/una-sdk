/**
 ******************************************************************************
 * @file    Model.hpp
 * @brief   GUI-side state of Images and its link to the kernel.
 *
 * The same role as the TouchGFX GUI's Model: it receives the kernel's
 * lifecycle callbacks (start, resume, suspend, stop) and is where a screen
 * asks for anything that involves the kernel or the service. Here that is
 * only exiting the app. With LVGL the model registers with SDK::LVGL::Port
 * instead of the TouchGFX command processor, and there is no UIEventListener
 * base: LVGL delivers button codes to the active screen as LV_EVENT_KEY.
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
    void onFrame() override;
    void onResume() override;
    void onSuspend() override;
    void onStop() override;
};

#endif // MODEL_HPP
