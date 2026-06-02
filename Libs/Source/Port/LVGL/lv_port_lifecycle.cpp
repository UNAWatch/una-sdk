/**
 ******************************************************************************
 * @file    lv_port_lifecycle.cpp
 * @brief   LVGL v9 lifecycle + tick + render loop for the Una-Watch kernel GUI.
 *
 * Owns the IGuiLifeCycleCallback implementation (GuiLifeCycle) and the GUI render
 * loop (lvgl_taskEntry). The frame clock is the kernel's EVENT_GUI_TICK consumed
 * via TouchGFXCommandProcessor::waitForFrameTick(); lv_tick_inc() is fed once per
 * frame from onFrame(). No LVGL-side VSync/OS timer (Design.md §1.4).
 ******************************************************************************
 */

#include "SDK/Port/LVGL/lv_port_lifecycle.h"

#define LOG_MODULE_PRX   "lv_port_lifecycle"
#define LOG_MODULE_LEVEL LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

#include "lvgl.h"

#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp"

/* Frame rate (== 10) — same divisor GuiConfig.hpp uses in GUI_CONFIG_MS_2_TICKS.
   Use the SDK-level constant (always on the SDK include path); App::kFrameRate
   (AppTypes.hpp) and SDK::GUI::Config::kFrameRate are identical (10). */
#include "SDK/GUI/Config.hpp"

/* Core GUI types owned by the core/screen foundation tasks (Software/App/LVGL-GUI/core).
   The LVGL-GUI include dirs are added to the GUI build (Design.md §7). */
#include "ScreenManager.hpp"
#include "Model.hpp"

/* Tick period in ms: 1000 / kFrameRate. TODO-TICK: a delta-based tick from
   EventGuiTick.timestamp could replace this once its units are confirmed against
   the kernel tick source; the first pass uses a fixed period for determinism. */
#ifndef UNA_LV_TICK_PERIOD_MS
#define UNA_LV_TICK_PERIOD_MS (1000u / SDK::GUI::Config::kFrameRate)
#endif

namespace
{

/**
 * @brief IGuiLifeCycleCallback implementation driving the LVGL screen lifecycle.
 */
class GuiLifeCycle : public SDK::Interface::IGuiLifeCycleCallback
{
public:
    void onStart() override
    {
        // Build + load the splash screen (registers all screens internally).
        una::gui::ScreenManager::instance().start();
    }

    void onStop() override
    {
        // Delete active screen + free GUI resources.
        una::gui::ScreenManager::instance().shutdown();
    }

    void onResume() override
    {
        // Rendering is gated by the transport anyway; LVGL keeps ticking.
        m_resumed = true;
    }

    void onSuspend() override
    {
        m_resumed = false;
    }

    void onFrame() override
    {
        // Advance LVGL time once per EVENT_GUI_TICK (fixed period, first pass).
        lv_tick_inc(UNA_LV_TICK_PERIOD_MS);
    }

private:
    bool m_resumed = false;
};

GuiLifeCycle g_life_cycle;

} // namespace

extern "C" void lv_port_lifecycle_init(void)
{
    SDK::TouchGFXCommandProcessor::GetInstance().setAppLifeCycleCallback(&g_life_cycle);
}

extern "C" void lvgl_taskEntry(void)
{
    auto& cp = SDK::TouchGFXCommandProcessor::GetInstance();
    for (;;) {
        // BLOCKS until EVENT_GUI_TICK (also demuxes lifecycle/buttons/custom msgs).
        // Returns false => render a frame; true => app is stopping (won't return here).
        if (cp.waitForFrameTick()) {
            continue; // STOP path: onStop already ran, sys.exit pending.
        }
        cp.callCustomMessageHandler();  // drain app-specific msgs before Model tick
        una::gui::Model::instance().tick();  // drain backend events -> push to active screen
        lv_timer_handler();              // runs animations + triggers flush_cb
    }
}
