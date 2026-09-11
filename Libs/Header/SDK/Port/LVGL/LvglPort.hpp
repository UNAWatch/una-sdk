/**
 ******************************************************************************
 * @file    LvglPort.hpp
 * @brief   LVGL v9 port for UNA SDK GUI applications.
 *
 * Binds LVGL to the kernel's GUI-process contract:
 *
 *   display  - LVGL renders RGB565 into a stripe buffer; each flushed stripe is
 *              packed into an 8-bit ABGR2222 frame, and when the last stripe of
 *              a frame lands the frame is sent to the kernel with
 *              REQUEST_DISPLAY_UPDATE.
 *   tick     - lv_tick uses the kernel's millisecond clock, so animation timing
 *              is correct even when GUI ticks arrive unevenly.
 *   frames   - the kernel's EVENT_GUI_TICK (10 Hz) gates one lv_timer_handler()
 *              call, so LVGL renders at most one frame per tick.
 *   buttons  - each SDK::GUI::Button code the kernel delivers is posted to the
 *              active screen as an LV_EVENT_KEY event. Screens read it with
 *              lv_event_get_key(). No LVGL input device or group is involved:
 *              a four-button watch has app-defined key meanings, which the
 *              focus-navigation model of an LVGL keypad group does not fit.
 *   lifecycle- COMMAND_APP_GUI_RESUME/SUSPEND/STOP reach the app through the
 *              IGuiLifeCycleCallback it registers here; on resume the active
 *              screen is invalidated so the kernel gets a full frame back.
 *
 * The message pump underneath is SDK::GuiCommandProcessor, shared with the
 * TouchGFX port: it demultiplexes kernel messages, queues button codes and
 * sends frame buffers, which is exactly what any GUI toolkit on this platform
 * needs.
 ******************************************************************************
 */

#ifndef SDK_PORT_LVGL_LVGL_PORT_HPP
#define SDK_PORT_LVGL_LVGL_PORT_HPP

#include <cstdint>

#include "lvgl.h"

#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Interfaces/ICustomMessageHandler.hpp"

namespace SDK::LVGL
{

/// Display geometry fixed by the kernel surface.
inline constexpr int32_t kDisplayWidth  = 240;
inline constexpr int32_t kDisplayHeight = 240;

/// Rows rendered per LVGL stripe. The stripe buffer is this many RGB565 rows;
/// the packed 8-bit frame always covers the whole display.
inline constexpr int32_t kStripeRows = 30;

/**
 * @brief The single LVGL port instance for this GUI process.
 */
class Port : public Interface::IGuiLifeCycleCallback
{
public:
    static Port& GetInstance();

    /**
     * @brief Initialise LVGL: lv_init(), the display, the tick source and logging.
     *
     * Call once from main() before the app creates any LVGL object.
     */
    void init();

    /**
     * @brief Register the app object that receives lifecycle callbacks.
     *
     * The port itself owns the callback slot in the kernel message pump and
     * forwards every event to this object after its own housekeeping.
     */
    void setAppLifeCycleCallback(Interface::IGuiLifeCycleCallback* cb) { mApp = cb; }

    /**
     * @brief Register the handler for application-private messages (service -> GUI).
     */
    void setCustomMessageHandler(Interface::ICustomMessageHandler* handler);

    /**
     * @brief Frame loop: one lv_timer_handler() per kernel tick.
     *
     * On the watch this never returns: the process ends inside the kernel's
     * COMMAND_APP_STOP handling or when the app calls sys.exit(). In the
     * simulator sys.exit() is a request to stop, and run() returns once the
     * loop has seen it so the host process can shut down.
     */
    void run();

    /// Make run() return after the current iteration (see sys.exit() above).
    void stop() { mStopped = true; }

    /**
     * @brief Host hook run once per loop iteration, before LVGL.
     *
     * Unused on the watch. The simulator uses it to pump window events and
     * show the last frame; it runs on the GUI thread, so it may call into
     * LVGL and the port.
     */
    using FrameHook = void (*)(void* ctx);
    void setFrameHook(FrameHook hook, void* ctx) { mFrameHook = hook; mFrameHookCtx = ctx; }

    /// The last frame sent to the kernel: kDisplayWidth * kDisplayHeight
    /// ABGR2222 bytes, row-major.
    const uint8_t* frame() const;

    /// Number of frames sent so far; a host compares it to detect a new frame.
    uint32_t frameCount() const { return mFrameCount; }

    /// True between onResume() and onSuspend(): the app owns the display.
    bool isResumed() const { return mResumed; }

    lv_display_t* display() const { return mDisplay; }

private:
    Port() = default;
    Port(const Port&)            = delete;
    Port& operator=(const Port&) = delete;

    // IGuiLifeCycleCallback (kernel -> port -> app)
    void onStart() override;
    void onStop() override;
    void onFrame() override;
    void onResume() override;
    void onSuspend() override;

    /// Post every queued button code to the active screen as LV_EVENT_KEY.
    void dispatchKeys();

    static void     flushCb(lv_display_t* disp, const lv_area_t* area, uint8_t* pxMap);
    static uint32_t tickCb();
    static void     logCb(lv_log_level_t level, const char* buf);

    lv_display_t*                     mDisplay      = nullptr;
    Interface::IGuiLifeCycleCallback* mApp          = nullptr;
    bool                              mResumed      = false;
    bool                              mStopped      = false;
    FrameHook                         mFrameHook    = nullptr;
    void*                             mFrameHookCtx = nullptr;
    uint32_t                          mFrameCount   = 0;
};

} // namespace SDK::LVGL

#endif // SDK_PORT_LVGL_LVGL_PORT_HPP
