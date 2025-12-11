/**
 ******************************************************************************
 * @file    IGuiLifeCycleCallback.hpp
 * @date    04-02-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Interface for a GUI application lifecycle.
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include <cstdint>

namespace SDK::Interface
{

/**
 * @brief Callback interface for application lifecycle events.
 */
class IGuiLifeCycleCallback {
public:

    /**
     * @brief Called before first GUI tick.
     * @note  State: Ticks enabled, no keys, no display.
     */
    virtual void onStart() {}

    /**
     * @brief Called to terminate the application.
     *
     * @note  The application MUST free all resources.
     *        After exiting this callback, the application
     *        will be destroyed.
     *
     * @note  State: No ticks, no keys, no display.
     */
    virtual void onStop() {}

    /**
     * @brief Called on every frame tick.
     */
    virtual void onFrame() {}

    /**
     * @brief Called when the kernel switches display focus to this application.
     * @note  State: Ticks enabled, keys enabled, display active.
     */
    virtual void onResume() {}

    /**
     * @brief Called when the kernel switches display focus to another task.
     * @note  Can be triggered after 'onResume()'.
     * @note  State: Ticks enabled, no keys, no display.
     */
    virtual void onSuspend() {}

protected:

    /**
     * @brief Destructor.
     */
    virtual ~IGuiLifeCycleCallback() = default;
};

} // namespace SDK::Interface
