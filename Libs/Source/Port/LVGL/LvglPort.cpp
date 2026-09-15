/**
 ******************************************************************************
 * @file    LvglPort.cpp
 * @brief   LVGL v9 port for UNA SDK GUI applications (see LvglPort.hpp).
 ******************************************************************************
 */

#include "SDK/Port/LVGL/LvglPort.hpp"
#include "SDK/Port/LVGL/LvglAssert.h"

#define LOG_MODULE_PRX      "LvglPort"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include <cstdlib>

#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Port/GuiCommandProcessor.hpp"

/// Build with -DUNA_LVGL_FRAME_STATS=1 to log, every 100 kernel ticks, how many
/// frames reached the kernel and how evenly the ticks arrived.
#ifndef UNA_LVGL_FRAME_STATS
#define UNA_LVGL_FRAME_STATS 0
#endif

namespace SDK::LVGL
{

namespace
{

#if UNA_LVGL_FRAME_STATS
struct FrameStats {
    uint32_t windowStartMs = 0;
    uint32_t firstFrameNo  = 0;   // kernel tick numbers covered by the window
    uint32_t lastFrameNo   = 0;
    uint32_t lastTickMs    = 0;
    uint32_t ticks         = 0;
    uint32_t frames        = 0;
    uint32_t minDtMs       = UINT32_MAX;
    uint32_t maxDtMs       = 0;
    uint32_t zeroDt        = 0;   // ticks that arrived together (a catch-up burst)
    uint32_t lateDt        = 0;   // gaps of more than 1.5 periods
    uint32_t maxRenderMs   = 0;   // longest lv_timer_handler() call
    uint32_t maxSendMs     = 0;   // longest frame hand-off to the kernel
    uint32_t sumRenderMs   = 0;
} sStats;

void statsOnTick(uint32_t nowMs, uint32_t frameNo)
{
    sStats.lastFrameNo = frameNo;
    if (sStats.ticks == 0) {
        sStats.windowStartMs = nowMs;
        sStats.firstFrameNo  = frameNo;
    } else {
        const uint32_t dt = nowMs - sStats.lastTickMs;
        sStats.minDtMs = LV_MIN(sStats.minDtMs, dt);
        sStats.maxDtMs = LV_MAX(sStats.maxDtMs, dt);
        if (dt == 0) {
            ++sStats.zeroDt;
        }
        if (dt > 150) {
            ++sStats.lateDt;
        }
    }
    sStats.lastTickMs = nowMs;
    if (++sStats.ticks == 100) {
        LOG_INFO("ticks #%u-%u: 100 in %u ms (dt %u..%u ms, %u at 0 ms, %u over 150 ms): %u frames sent, "
                 "render max %u avg %u ms, send max %u ms\n",
                 static_cast<unsigned>(sStats.firstFrameNo), static_cast<unsigned>(sStats.lastFrameNo),
                 static_cast<unsigned>(nowMs - sStats.windowStartMs),
                 static_cast<unsigned>(sStats.minDtMs), static_cast<unsigned>(sStats.maxDtMs),
                 static_cast<unsigned>(sStats.zeroDt), static_cast<unsigned>(sStats.lateDt),
                 static_cast<unsigned>(sStats.frames),
                 static_cast<unsigned>(sStats.maxRenderMs), static_cast<unsigned>(sStats.sumRenderMs / 100u),
                 static_cast<unsigned>(sStats.maxSendMs));
        sStats = FrameStats{};
    }
}
#endif

/// The frame the kernel consumes: one ABGR2222 byte per pixel, row-major,
/// A in bits 7..6, B in 5..4, G in 3..2, R in 1..0. Persistent across frames
/// so LVGL's partial redraws compose onto the previous content.
uint8_t sFrame[kDisplayWidth * kDisplayHeight];

/// LVGL's render target: RGB565 rows, reused for every stripe of a frame.
alignas(LV_DRAW_BUF_ALIGN) uint8_t sStripe[kDisplayWidth * kStripeRows * 2];

/// Pack one RGB565 pixel to ABGR2222 by keeping the top two bits of each channel.
/// Same quantisation TouchGFX's LCD8bpp_ABGR2222 applies, so both toolkits
/// produce identical colours from identical input.
inline uint8_t rgb565ToAbgr2222(uint16_t px)
{
    const uint8_t r2 = static_cast<uint8_t>((px >> 14) & 0x03);   // R bits 15..14
    const uint8_t g2 = static_cast<uint8_t>((px >> 9)  & 0x03);   // G bits 10..9
    const uint8_t b2 = static_cast<uint8_t>((px >> 3)  & 0x03);   // B bits  4..3
    return static_cast<uint8_t>(0xC0 | (b2 << 4) | (g2 << 2) | r2);
}

} // namespace

Port& Port::GetInstance()
{
    static Port sInstance;
    return sInstance;
}

void Port::init()
{
    lv_init();

    lv_tick_set_cb(&Port::tickCb);
    lv_log_register_print_cb(&Port::logCb);

    mDisplay = lv_display_create(kDisplayWidth, kDisplayHeight);
    lv_display_set_color_format(mDisplay, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(mDisplay, sStripe, nullptr, sizeof(sStripe), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(mDisplay, &Port::flushCb);
    lv_display_set_default(mDisplay);

    SDK::GuiCommandProcessor::GetInstance().setAppLifeCycleCallback(this);

    LOG_INFO("LVGL %d.%d.%d ready, %dx%d, stripe %d rows\n",
             LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH,
             kDisplayWidth, kDisplayHeight, kStripeRows);
}

void Port::setCustomMessageHandler(Interface::ICustomMessageHandler* handler)
{
    SDK::GuiCommandProcessor::GetInstance().setCustomMessageHandler(handler);
}

void Port::run()
{
    auto& pump = SDK::GuiCommandProcessor::GetInstance();

    while (!mStopped) {
        // Blocks until the kernel's next GUI tick. Lifecycle commands and button
        // events are handled inside; app-private messages are queued.
        if (pump.waitForFrameTick()) {
            break;      // Stop path: onStop() has run and sys.exit() was called.
        }

        if (mFrameHook) {
            mFrameHook(mFrameHookCtx);
        }

        // The simulator build also wakes on a poll timeout; only a kernel tick
        // advances the GUI, so the frame rate is the watch's in both places.
        if (!pump.consumeFrameTick()) {
            continue;
        }

        // Deliver queued service -> GUI messages, then buttons, then let LVGL
        // run its timers and render whatever the two invalidated.
        pump.callCustomMessageHandler();
        dispatchKeys();
#if UNA_LVGL_FRAME_STATS
        const uint32_t t0 = tickCb();
        statsOnTick(t0, pump.lastFrameNumber());
        lv_timer_handler();
        const uint32_t renderMs = tickCb() - t0;
        sStats.maxRenderMs = LV_MAX(sStats.maxRenderMs, renderMs);
        sStats.sumRenderMs += renderMs;
#else
        lv_timer_handler();
#endif
    }
}

void Port::dispatchKeys()
{
    // One code per frame, as TouchGFX samples them. A press often asks for a
    // screen switch, which the app performs at the end of this frame (see
    // lv_async_call); delivering the click and release queued behind it in
    // the same frame would hand them to the screen being left, and a screen
    // that waits for the release (hold to confirm) would never see it.
    auto&   pump = SDK::GuiCommandProcessor::GetInstance();
    uint8_t code = 0;
    if (!pump.getKeySample(code)) {
        return;
    }
    lv_obj_t* screen = lv_screen_active();
    if (!screen) {
        return;
    }
    uint32_t key = code;
    lv_obj_send_event(screen, LV_EVENT_KEY, &key);
}

// --- IGuiLifeCycleCallback -------------------------------------------------

void Port::onStart()
{
    if (mApp) {
        mApp->onStart();
    }
}

void Port::onStop()
{
    if (mApp) {
        mApp->onStop();
    }
    lv_deinit();
    mStopped = true;
}

const uint8_t* Port::frame() const
{
    return sFrame;
}

void Port::onFrame()
{
    if (mApp) {
        mApp->onFrame();
    }
}

void Port::onResume()
{
    mResumed = true;
    // Another GUI may have owned the display meanwhile; the kernel needs a full
    // frame from us, and LVGL only redraws what is invalid.
    if (lv_obj_t* screen = lv_screen_active()) {
        lv_obj_invalidate(screen);
    }
    if (mApp) {
        mApp->onResume();
    }
}

void Port::onSuspend()
{
    mResumed = false;
    if (mApp) {
        mApp->onSuspend();
    }
}

// --- LVGL callbacks --------------------------------------------------------

void Port::flushCb(lv_display_t* disp, const lv_area_t* area, uint8_t* pxMap)
{
    const int32_t x1 = LV_MAX(area->x1, 0);
    const int32_t x2 = LV_MIN(area->x2, kDisplayWidth - 1);
    const int32_t y1 = LV_MAX(area->y1, 0);
    const int32_t y2 = LV_MIN(area->y2, kDisplayHeight - 1);

    // In PARTIAL mode pxMap is a tightly packed image of `area`.
    const int32_t   srcStride = lv_area_get_width(area);
    const uint16_t* src       = reinterpret_cast<const uint16_t*>(pxMap);

    for (int32_t y = y1; y <= y2; ++y) {
        const uint16_t* row = src + (y - area->y1) * srcStride + (x1 - area->x1);
        uint8_t*        dst = &sFrame[y * kDisplayWidth + x1];
        for (int32_t x = x1; x <= x2; ++x) {
            *dst++ = rgb565ToAbgr2222(*row++);
        }
    }

    if (lv_display_flush_is_last(disp)) {
        // Sends REQUEST_DISPLAY_UPDATE; a no-op while the GUI is suspended.
#if UNA_LVGL_FRAME_STATS
        const uint32_t t0 = tickCb();
        SDK::GuiCommandProcessor::GetInstance().writeDisplayFrameBuffer(sFrame);
        sStats.maxSendMs = LV_MAX(sStats.maxSendMs, tickCb() - t0);
        ++sStats.frames;
#else
        SDK::GuiCommandProcessor::GetInstance().writeDisplayFrameBuffer(sFrame);
#endif
        ++GetInstance().mFrameCount;
    }

    lv_display_flush_ready(disp);
}

uint32_t Port::tickCb()
{
    // LVGL expects a tick that wraps at 2^32. The kernel clock returns to 0
    // short of that (after about 48 days), which lv_tick_elaps() would read
    // as a huge step that ends every animation and fires every timer at
    // once, so accumulate the clock's deltas into a tick of our own.
    static uint32_t sLast = 0;
    static uint32_t sTick = 0;
    const uint32_t  now   = SDK::KernelProviderGUI::GetInstance().getKernel().sys.getTimeMs();
    sTick += (now >= sLast) ? (now - sLast) : now;
    sLast = now;
    return sTick;
}

void Port::logCb(lv_log_level_t level, const char* buf)
{
    switch (level) {
        case LV_LOG_LEVEL_ERROR: LOG_ERROR("%s\n", buf);   break;
        case LV_LOG_LEVEL_WARN:  LOG_WARNING("%s\n", buf); break;
        case LV_LOG_LEVEL_USER:
        case LV_LOG_LEVEL_INFO:  LOG_INFO("%s\n", buf);    break;
        default:                 LOG_DEBUG("%s\n", buf);   break;
    }
}

} // namespace SDK::LVGL

extern "C" void una_lvgl_assert_failed(const char* file, int line)
{
    SDK::KernelProviderGUI::GetInstance().getKernel().log.printf(
        "-E- LVGL assert failed at %s:%d\n", file ? file : "?", line);
    exit(-1);
}

/**
 * @brief Fallback for LV_FONT_DEFAULT (see lv_conf.h).
 *
 * Weak: an app that defines its own una_lvgl_default_font() returning one of
 * its converted fonts replaces this, and Montserrat 14 then drops out of the
 * link entirely. MSVC (the PC simulator) has no weak symbols; its linker's
 * /alternatename gives the same "use this unless defined elsewhere" rule.
 */
#if defined(_MSC_VER)
extern "C" const lv_font_t* una_lvgl_default_font_fallback(void)
{
    return &lv_font_montserrat_14;
}
#if defined(_M_IX86)
#pragma comment(linker, "/alternatename:_una_lvgl_default_font=_una_lvgl_default_font_fallback")
#else
#pragma comment(linker, "/alternatename:una_lvgl_default_font=una_lvgl_default_font_fallback")
#endif
#else
extern "C" __attribute__((weak)) const lv_font_t* una_lvgl_default_font(void)
{
    return &lv_font_montserrat_14;
}
#endif
