/**
 ******************************************************************************
 * @file    lv_port_disp.cpp
 * @brief   LVGL v9 display port for the Una-Watch kernel GUI.
 *
 * Single 240x240 display, FULL render mode, single RGB565 draw buffer. The flush
 * callback is THE transport seam: it down-converts the RGB565 render buffer to the
 * 8bpp ABGR2222 transfer buffer the frozen transport consumes, then forwards to
 * SDK::TouchGFXCommandProcessor::writeDisplayFrameBuffer (Design.md §1.2).
 ******************************************************************************
 */

#include "SDK/Port/LVGL/lv_port_disp.h"

#define LOG_MODULE_PRX   "lv_port_disp"
#define LOG_MODULE_LEVEL LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp"

/* lv_conf.h supplies UNA_LV_* helper macros (via LV_CONF_PATH). Mirror as a
   defensive fallback so this file is self-contained if conf macros are absent. */
#ifndef UNA_LV_HOR_RES
#define UNA_LV_HOR_RES 240
#endif
#ifndef UNA_LV_VER_RES
#define UNA_LV_VER_RES 240
#endif
#ifndef UNA_LV_BYTES_PER_PX
#define UNA_LV_BYTES_PER_PX 2
#endif
#ifndef UNA_LV_COLOR_FORMAT
#define UNA_LV_COLOR_FORMAT LV_COLOR_FORMAT_RGB565
#endif

/* Pixel counts / buffer sizes. */
static const uint32_t kPixelCount = (uint32_t)UNA_LV_HOR_RES * (uint32_t)UNA_LV_VER_RES; /* 57600 */

/* LVGL renders here in RGB565 (2 B/px) => 115200 B. */
static uint8_t s_draw_buf[UNA_LV_HOR_RES * UNA_LV_VER_RES * UNA_LV_BYTES_PER_PX];

/* The frozen transport consumes THIS: 8bpp ABGR2222 (1 B/px) => 57600 B. */
static uint8_t s_xfer_buf[UNA_LV_HOR_RES * UNA_LV_VER_RES];

/**
 * @brief Convert one RGB565 pixel to one ABGR2222 byte.
 *
 * Bit layout: A[7:6] B[5:4] G[3:2] R[1:0]. Opaque on-screen frame => alpha = 0b11.
 * The 5/6-bit -> 2-bit truncation (drop low bits) matches TouchGFX's own
 * quantization (the dropped PainterABGR2222 / LCD8bpp_ABGR2222 layout).
 *
 * TODO-CF-VERIFY (cosmetic only): confirm the panel's opaque-pixel alpha-bit
 * interpretation on hardware. Frame buffers are normally fully opaque (A=0b11).
 */
static inline uint8_t una_rgb565_to_abgr2222(uint16_t c)
{
    uint8_t r5 = (uint8_t)((c >> 11) & 0x1F);   /* R: bits 15..11 */
    uint8_t g6 = (uint8_t)((c >> 5)  & 0x3F);   /* G: bits 10..5  */
    uint8_t b5 = (uint8_t)( c        & 0x1F);   /* B: bits  4..0  */
    uint8_t r2 = (uint8_t)(r5 >> 3);            /* 5 -> 2 bits    */
    uint8_t g2 = (uint8_t)(g6 >> 4);            /* 6 -> 2 bits    */
    uint8_t b2 = (uint8_t)(b5 >> 3);            /* 5 -> 2 bits    */
    return (uint8_t)((0x3u << 6) | (b2 << 4) | (g2 << 2) | r2); /* A=3, B, G, R */
}

/**
 * @brief Pack the full RGB565 render buffer into the ABGR2222 transfer buffer.
 *        The SINGLE place color conversion happens. NOT an identity pass.
 */
static void una_lv_pack_framebuffer(const uint8_t* src565, uint8_t* dst8)
{
    /* src565 is little-endian 16-bit pixels as laid out by LVGL's RGB565 draw buffer. */
    const uint16_t* src = reinterpret_cast<const uint16_t*>(src565);
    for (uint32_t i = 0; i < kPixelCount; ++i) {
        dst8[i] = una_rgb565_to_abgr2222(src[i]);
    }
}

/**
 * @brief LVGL flush callback. FULL render mode => one call per frame covering the
 *        whole screen. px_map is the RGB565 render buffer.
 */
static void una_lv_flush_cb(lv_display_t* disp, const lv_area_t* /*area*/, uint8_t* px_map)
{
    una_lv_pack_framebuffer(px_map, s_xfer_buf);    /* RGB565 (2 B/px) -> ABGR2222 (1 B/px) */

    /* writeDisplayFrameBuffer already early-returns when GUI is suspended or data==NULL,
       so we do NOT re-check resumed state here (Design.md §1.2). */
    SDK::TouchGFXCommandProcessor::GetInstance().writeDisplayFrameBuffer(s_xfer_buf);

    lv_display_flush_ready(disp);
}

extern "C" lv_display_t* lv_port_disp_init(void)
{
    lv_display_t* disp = lv_display_create(UNA_LV_HOR_RES, UNA_LV_VER_RES);
    if (!disp) {
        LOG_ERROR("lv_display_create failed\n");
        return nullptr;
    }

    lv_display_set_color_format(disp, UNA_LV_COLOR_FORMAT);

    /* Single buffer, FULL render mode — mirrors the single-buffer/full-flush TouchGFX port. */
    lv_display_set_buffers(disp,
                           s_draw_buf,
                           NULL,
                           sizeof(s_draw_buf),
                           LV_DISPLAY_RENDER_MODE_FULL);

    lv_display_set_flush_cb(disp, una_lv_flush_cb);
    lv_display_set_default(disp);

    /* TODO-ROT: first pass assumes ROTATION_0. If the panel/transport expects a
       rotated buffer, set lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90). */

    return disp;
}
