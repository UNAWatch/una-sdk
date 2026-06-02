/**
 ******************************************************************************
 * @file    lv_conf.h
 * @brief   LVGL v9 configuration for the Una-Watch kernel GUI port.
 *
 * Canonical location: SDK/Libs/Header/SDK/Port/LVGL/lv_conf.h
 * Enabled via the build define LV_CONF_PATH=<this path> (see Design.md §5.4 / §7).
 *
 * Frozen invariants (LVGL Migration Contract):
 *   - Fixed 240x240, rotation 0.
 *   - LVGL renders RGB565 (LV_COLOR_DEPTH 16); the flush_cb down-converts to the
 *     8bpp ABGR2222 transport buffer in lv_port_disp.cpp (NOT here).
 *   - LV_USE_OS == LV_OS_NONE: the kernel frame clock drives lv_timer_handler();
 *     lv_tick_inc() is fed from onFrame() (EVENT_GUI_TICK). No LVGL VSync/OS thread.
 *   - Keypad indev only (no touch / pointer).
 ******************************************************************************
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

/* RGB565 render buffer; packed down to 8bpp ABGR2222 on flush (Design.md §1.2). */
#define LV_COLOR_DEPTH 16

/*=========================
   STDLIB WRAPPER SETTINGS
 *=========================*/

/* Use LVGL's built-in allocator over a static pool (no malloc on the MCU heap). */
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_BUILTIN

#define LV_STDINT_INCLUDE       <stdint.h>
#define LV_STDDEF_INCLUDE       <stddef.h>
#define LV_STDBOOL_INCLUDE      <stdbool.h>
#define LV_INTTYPES_INCLUDE     <inttypes.h>
#define LV_LIMITS_INCLUDE       <limits.h>
#define LV_STDARG_INCLUDE       <stdarg.h>

/* TODO-MEM: size against the kernel-GUI widget budget. Default 48-64 KB (Design.md §5.4). */
#define LV_MEM_SIZE             (64U * 1024U)
#define LV_MEM_POOL_EXPAND_SIZE 0
#define LV_MEM_ADR              0

/*====================
   HAL SETTINGS
 *====================*/

/* We feed lv_tick_inc() ourselves from onFrame() (Design.md §1.4). No custom tick getter. */
#define LV_DEF_REFR_PERIOD      30   /* informational; the kernel gates the real frame cadence */
#define LV_DPI_DEF              130

/*=================
   OPERATING SYSTEM
 *=================*/

/* No LVGL-owned OS thread; the kernel GUI task drives lv_timer_handler(). */
#define LV_USE_OS               LV_OS_NONE

/*========================
   RENDERING CONFIGURATION
 *========================*/

/* Software rendering only. TODO-GPU: STM32U5 GPU2D acceleration is out of scope. */
#define LV_USE_DRAW_SW          1
#define LV_DRAW_SW_SUPPORT_RGB565   1
#define LV_DRAW_SW_SUPPORT_RGB888   1
#define LV_DRAW_SW_SUPPORT_ARGB8888 1
#define LV_DRAW_SW_SUPPORT_L8       1
#define LV_DRAW_SW_SUPPORT_AL88     1
#define LV_DRAW_SW_SUPPORT_A8       1
#define LV_DRAW_SW_SUPPORT_I1       1
#define LV_DRAW_SW_DRAW_UNIT_CNT    1
#define LV_DRAW_SW_COMPLEX          1

/*=======================
   FEATURE CONFIGURATION
 *=======================*/

/* Logging (Design.md §5.4: LV_USE_LOG 1). */
#define LV_USE_LOG              1
#if LV_USE_LOG
    #define LV_LOG_LEVEL        LV_LOG_LEVEL_WARN
    #define LV_LOG_PRINTF       0
    #define LV_LOG_USE_TIMESTAMP 0
    #define LV_LOG_USE_FILE_LINE 1
#endif

#define LV_USE_ASSERT_NULL      1
#define LV_USE_ASSERT_MALLOC    1
#define LV_USE_ASSERT_STYLE     0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ       0
#define LV_ASSERT_HANDLER_INCLUDE <stdint.h>
#define LV_ASSERT_HANDLER       while(1);

/*==================
   FONT USAGE
 *==================*/

/* Built-in Montserrat fonts: enable only the sizes actually used (Design.md §5.4 / §5.2).
   Custom committed Poppins faces live under generated/fonts (owned by the theme task). */
#define LV_FONT_MONTSERRAT_14   1   /* LVGL default fallback font; keep at least one builtin. */
#define LV_FONT_MONTSERRAT_18   1
#define LV_FONT_MONTSERRAT_20   1
#define LV_FONT_MONTSERRAT_28   1

#define LV_FONT_DEFAULT         &lv_font_montserrat_14

#define LV_FONT_FMT_TXT_LARGE   0
#define LV_USE_FONT_COMPRESSED  0
#define LV_USE_FONT_PLACEHOLDER 1

/*=================
   TEXT SETTINGS
 *=================*/

#define LV_TXT_ENC              LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS      " ,.;:-_)]}"
#define LV_TXT_LINE_BREAK_LONG_LEN 0
#define LV_USE_BIDI             0
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*==================
   WIDGET USAGE
 *==================*/

#define LV_USE_ARC              1
#define LV_USE_LABEL            1
#if LV_USE_LABEL
    #define LV_LABEL_TEXT_SELECTION 0
    #define LV_LABEL_LONG_TXT_HINT  1
    #define LV_LABEL_WAIT_CHAR_COUNT 3
#endif
#define LV_USE_IMAGE            1
#define LV_USE_LINE             1
#define LV_USE_SWITCH           1
#define LV_USE_ROLLER           1

/* Other widgets — disabled (kernel GUI uses only the set above + base obj). */
#define LV_USE_ANIMIMG          0
#define LV_USE_BAR              0
#define LV_USE_BUTTON           0
#define LV_USE_BUTTONMATRIX     0
#define LV_USE_CANVAS           0
#define LV_USE_CHECKBOX         0
#define LV_USE_DROPDOWN         0
#define LV_USE_IMAGEBUTTON      0
#define LV_USE_KEYBOARD         0
#define LV_USE_LED              0
#define LV_USE_LIST             0
#define LV_USE_MENU             0
#define LV_USE_MSGBOX           0
#define LV_USE_SCALE            0
#define LV_USE_SLIDER           0
#define LV_USE_SPAN             0
#define LV_USE_SPINBOX          0
#define LV_USE_SPINNER          0
#define LV_USE_TABLE            0
#define LV_USE_TABVIEW          0
#define LV_USE_TEXTAREA         0
#define LV_USE_TILEVIEW         0
#define LV_USE_WIN              0

/*==================
   THEME USAGE
 *==================*/

/* No prebuilt theme; the Una-Watch Theme module builds shared styles (Design.md §5.1). */
#define LV_USE_THEME_DEFAULT    0
#define LV_USE_THEME_SIMPLE     0
#define LV_USE_THEME_MONO       0

/*==================
   LAYOUTS
 *==================*/

#define LV_USE_FLEX             1
#define LV_USE_GRID             1

/*====================
   DRAW / MISC
 *====================*/

#define LV_USE_OBJ_NAME         0
#define LV_USE_USER_DATA        1
#define LV_ENABLE_GLOBAL_CUSTOM 0
#define LV_USE_PERF_MONITOR     0
#define LV_USE_MEM_MONITOR      0
#define LV_USE_REFR_DEBUG       0
#define LV_USE_SYSMON           0
#define LV_USE_PROFILER         0
#define LV_USE_MONKEY           0
#define LV_USE_GRIDNAV          0
#define LV_USE_FRAGMENT         0
#define LV_USE_IMGFONT          0
#define LV_USE_OBSERVER         1
#define LV_USE_FREETYPE         0
#define LV_USE_SNAPSHOT         0

/* Image decoders — none needed (assets are committed lv_image_dsc_t, Design.md §5.3). */
#define LV_USE_LODEPNG          0
#define LV_USE_LIBPNG           0
#define LV_USE_BMP              0
#define LV_USE_TJPGD            0
#define LV_USE_LIBJPEG_TURBO    0
#define LV_USE_GIF              0
#define LV_USE_QRCODE           0
#define LV_USE_BARCODE          0
#define LV_USE_FFMPEG           0

/* No demos / examples in the firmware build. */
#define LV_BUILD_EXAMPLES       0

/*========================
   UNA-WATCH PORT HELPERS
 *========================*/

/* Fixed display geometry (Design.md §1.2, frozen invariant 3). */
#define UNA_LV_HOR_RES          240
#define UNA_LV_VER_RES          240

/* LVGL RENDER format and its byte/pixel width (the DRAW buffer only).
   The TRANSPORT format is fixed at 8bpp ABGR2222 (1 byte/px); it is NOT a tunable.
   una_lv_pack_framebuffer() (lv_port_disp.cpp) bridges RGB565 render -> ABGR2222. */
#define UNA_LV_COLOR_FORMAT     LV_COLOR_FORMAT_RGB565
#define UNA_LV_BYTES_PER_PX     2

#endif /* LV_CONF_H */
