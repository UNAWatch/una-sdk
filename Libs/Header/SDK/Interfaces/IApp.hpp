/**
 ******************************************************************************
 * @file    IApp.hpp
 * @date    04-02-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Interface for a user application interacting with the kernel.
 *
 * This interface defines the lifecycle management and essential interactions
 * between the user application and the kernel, including display updates
 * input handling and generates notifications.
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include <cstdint>

#include "SDK/Interfaces/IGlance.hpp"

namespace SDK::Interface
{

/**
 * @brief Interface for an user application interacting with the kernel.
 *
 * This interface provides methods for managing the application's lifecycle,
 * handling display output, and processing user input events.
 */
class IApp {
public:

    /**
     * @brief Application frame rate for display updates.
     */
    static constexpr const uint32_t kFrameRate = 10;

    /**
     * @brief Reasons for application launch.
     */
    enum class LaunchReason {
        AUTO_START,
        ON_DEMAND
    };

    /**
     * @brief External button definitions.
     */
    enum Button : uint8_t
    {
        BUTTON_L1   = '1',
        BUTTON_L2   = '2',
        BUTTON_R1   = '3',
        BUTTON_R2   = '4',
        BUTTON_L1R2 = 'z',
    };

    /**
     * @brief Callback interface for application lifecycle events.
     */
    class Callback {
    public:
        /**
         * @brief Called after the application confirms initialization via 'initialized()'.
         * @note  State: No ticks, no keys, no display.
         */
        virtual void onCreate() {};

        /**
         * @brief Called after 'onCreate()' or after 'onStop()' to restart the application.
         * @note  State: Ticks enabled, no keys, no display.
         */
        virtual void onStart() {};

        /**
         * @brief Called when the kernel switches display focus to this application.
         * @note  Can be triggered after 'onStart()' or 'onPause()'.
         * @note  State: Ticks enabled, keys enabled, display active.
         */
        virtual void onResume() {};

        /**
         * @brief Called on every frame tick when the application is in the 'resume' state.
         */
        virtual void onFrame() {};

        /**
         * @brief Called when the kernel switches display focus to another task.
         * @note  Can be triggered after 'onResume()'.
         * @note  State: Ticks enabled, no keys, no display.
         */
        virtual void onPause() {};

        /**
         * @brief Called after 'onPause()' or 'onStart()' to terminate the application.
         * @note  State: No ticks, no keys, no display.
         */
        virtual void onStop() {};

        /**
         * @brief Called just before the application object is destroyed.
         */
        virtual void onDestroy() {};

        /**
         * @brief Called only for Service app after on GUI 'onStart()' to notify Service app
         *        about GUI state.
         * @note  Can be triggered only if Service app is started (e.g. after 'onStart()).
         */
        virtual void onStartGUI() {};

        /**
         * @brief Called only for Service app after before 'onStop()' to notify Service app 
         *        about GUI state.
         * @note  Can be triggered only if Service app is started (e.g. after 'onStart()).
         */
        virtual void onStopGUI() {};

    protected:
        /**
         * @brief Virtual destructor.
         */
        virtual ~Callback() = default;
    };

    /**
     * @brief Returns the cause of the App launch
     *
     */
    virtual LaunchReason getLaunchReason() = 0;

    /**
     * @brief Registers a lifecycle callback.
     *
     * The application can register a callback to receive lifecycle event notifications.
     * If no callback is registered, the application can still be suspended or destroyed,
     * but it will not be notified.
     *
     * @note The callback will not be triggered until the application calls 'initialized()'.
     * @param pCallback Pointer to the lifecycle callback implementation.
     */
    virtual void registerApp(Interface::IApp::Callback *pCallback) = 0;

    /**
     * @brief Registers a glance interface
     *
     * If the application can act as a glance it should to registry its glance interface
     *
     * @param glance Pointer to the Glance interface
     */
    virtual void registerGlance(SDK::Interface::IGlance* glance) = 0;

    /**
     * @brief Confirms that the application has completed initialization.
     *
     * Since the application runs in a separate thread, the kernel must wait until
     * initialization (such as static field setup) is complete. Once ready to receive
     * kernel events, the application should call this method.
     *
     * @note Alternatively, the application can call the blocking 'waitForFrame()'
     *       method, which also allows kernel interaction.
     */
    virtual void initialized() = 0;

    /**
     * @brief Blocks execution until the next frame tick.
     *
     * Some frameworks, such as TouchGFX, have their own processing loop.
     * This method synchronizes that loop with the kernel.
     *
     * @note While blocked in this method, the kernel can invoke callbacks,
     *       ensuring thread safety.
     * @note If 'onFrame()' is implemented, it is called before this method exits.
     */
    virtual void waitForFrame() = 0;

    /**
     * @brief Requests the kernel to switch display focus to another task.
     *
     * The kernel will call the 'onPause()' callback when the application is paused.
     */
    virtual void pauseRequest() = 0;

    /**
     * @brief Requests the kernel to switch display focus back to this application.
     *
     * The kernel will call the 'onResume()' callback when the application is resumed.
     * The kernel may ignore this request if there are other higher-priority tasks.
     */
    virtual void resumeRequest() = 0;

    /**
     * @brief Retrieves the display resolution.
     * @param width  Reference to store the display width.
     * @param height Reference to store the display height.
     */
    virtual void getDisplayResolution(int16_t &width, int16_t &height) = 0;

    /**
     * @brief Retrieves the display color depth.
     * @return Bits per pixel.
     */
    virtual uint8_t getDisplayColorDepth() = 0;

    /**
     * @brief Returns the glance area that is available for userapp
     * @param width  Reference to store the glance area width.
     * @param height Reference to store the glance area height.
     */
    virtual void getGlanceArea(int16_t &width, int16_t &height) = 0;

    /**
     * @brief Writes a frame buffer to the display.
     * @param pBuf Pointer to the frame buffer.
     *
     * @note The buffer size in bytes should be:
     *       width * height * (color depth + 7) / 8.
     */
    virtual void writeFrameBuffer(const uint8_t *pBuf) = 0;

    /**
     * @brief Samples external key events.
     * @param key Reference to store the detected key value (if any).
     *
     * @note This method can be called anytime to retrieve pressed keys from
     *       the kernel queue. However, key presses are only stored when the
     *       application is in the 'resume' state.
     *
     * @return True if a keypress was detected and stored in 'key'.
     */
    virtual bool keySample(uint8_t &key) = 0;


    //TODO: used in Sensor driver. Move to ISystem
    /**
     * @brief Locks of the application mutex
     *
     */
    virtual void lock() = 0;

    /**
     * @brief Unlocks of the application mutex
     *
     */
    virtual void unLock() = 0;

protected:
    /**
     * @brief Virtual destructor.
     */
    virtual ~IApp() = default;
};

} // namespace SDK::Interface
