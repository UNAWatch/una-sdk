/**
 ******************************************************************************
 * @file    lv_conf.h
 * @brief   LVGL v9 configuration for UNA SDK GUI applications.
 *
 * Selected at compile time through the LV_CONF_PATH define, which the SDK's
 * CMake sets to this file (see cmake/una-sdk.cmake, UNA_LVGL_CONF). An app that
 * needs a different configuration points UNA_LVGL_CONF at its own copy.
 *
 * Any option not set here takes LVGL's built-in default from
 * src/lv_conf_internal.h.
 *
 * Fixed facts of the UNA platform that this configuration encodes:
 *   - 240x240 display. LVGL renders RGB565; the port packs that down to the
 *     8-bit ABGR2222 frame the kernel accepts (see LvglPort.cpp).
 *   - No OS integration: the kernel's 10 Hz GUI tick drives lv_timer_handler()
 *     and the kernel clock feeds lv_tick_set_cb().
 *   - Buttons only, no touch. Key codes are the SDK::GUI::Button values.
 *   - Apps are freestanding: no heap growth (LVGL uses its own static pool)
 *     and no filesystem access from LVGL.
 ******************************************************************************
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

#define LV_COLOR_DEPTH 16

/*=========================
   STDLIB WRAPPER SETTINGS
 *=========================*/

/* LVGL's own allocator over a static pool. Apps get their heap from the kernel
 * with a per-process quota and per-block accounting, so keeping LVGL's many
 * small allocations inside one static block is both cheaper and predictable. */
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_BUILTIN

#define LV_STDINT_INCLUDE       <stdint.h>
#define LV_STDDEF_INCLUDE       <stddef.h>
#define LV_STDBOOL_INCLUDE      <stdbool.h>
#define LV_INTTYPES_INCLUDE     <inttypes.h>
#define LV_LIMITS_INCLUDE       <limits.h>
#define LV_STDARG_INCLUDE       <stdarg.h>

#define LV_MEM_SIZE             (40U * 1024U)
#define LV_MEM_POOL_EXPAND_SIZE 0
#define LV_MEM_ADR              0

/*====================
   HAL SETTINGS
 *====================*/

/* The kernel paces the GUI: it delivers one tick every 100 ms and the port
 * calls lv_timer_handler() once per tick. LVGL's refresh and animation timers
 * only run once their period has elapsed, so a period equal to the tick
 * (100 ms) skips every tick that lands a millisecond early and halves the
 * frame rate. 1 ms makes each tick render whatever is invalid. */
#define LV_DEF_REFR_PERIOD      1
#define LV_DPI_DEF              130

/*=================
   OPERATING SYSTEM
 *=================*/

#define LV_USE_OS               LV_OS_NONE

/*========================
   RENDERING CONFIGURATION
 *========================*/

#define LV_DRAW_BUF_STRIDE_ALIGN        1
#define LV_DRAW_BUF_ALIGN               4
#define LV_DRAW_LAYER_SIMPLE_BUF_SIZE   (16 * 1024)
#define LV_DRAW_LAYER_MAX_MEMORY        0

#define LV_USE_DRAW_SW 1
#if LV_USE_DRAW_SW
    /* Only the formats the port and committed assets use, to keep code size down. */
    #define LV_DRAW_SW_SUPPORT_RGB565                   1
    #define LV_DRAW_SW_SUPPORT_RGB565_SWAPPED           0
    #define LV_DRAW_SW_SUPPORT_RGB565A8                 1
    #define LV_DRAW_SW_SUPPORT_RGB888                   0
    #define LV_DRAW_SW_SUPPORT_XRGB8888                 0
    #define LV_DRAW_SW_SUPPORT_ARGB8888                 1
    #define LV_DRAW_SW_SUPPORT_ARGB8888_PREMULTIPLIED   0
    #define LV_DRAW_SW_SUPPORT_L8                       0
    #define LV_DRAW_SW_SUPPORT_AL88                     0
    #define LV_DRAW_SW_SUPPORT_A8                       1
    #define LV_DRAW_SW_SUPPORT_I1                       0

    #define LV_DRAW_SW_DRAW_UNIT_CNT    1
    #define LV_USE_DRAW_SW_ASM          LV_DRAW_SW_ASM_NONE
    #define LV_DRAW_SW_COMPLEX          1
    #define LV_USE_DRAW_SW_COMPLEX_GRADIENTS 0
#endif

/*=======================
   FEATURE CONFIGURATION
 *=======================*/

/* Logging goes to the kernel logger through the print callback the port registers. */
#define LV_USE_LOG 1
#if LV_USE_LOG
    #define LV_LOG_LEVEL            LV_LOG_LEVEL_WARN
    #define LV_LOG_PRINTF           0
    #define LV_LOG_USE_TIMESTAMP    0
    #define LV_LOG_USE_FILE_LINE    1
#endif

#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

/* A failed assert logs file/line and terminates the process, so the kernel
 * records it instead of the GUI silently spinning. */
#define LV_ASSERT_HANDLER_INCLUDE   "SDK/Port/LVGL/LvglAssert.h"
#define LV_ASSERT_HANDLER           una_lvgl_assert_failed(__FILE__, __LINE__);

#define LV_USE_PERF_MONITOR     0
#define LV_USE_MEM_MONITOR      0
#define LV_USE_REFR_DEBUG       0
#define LV_USE_LAYER_DEBUG      0
#define LV_USE_PARALLEL_DRAW_DEBUG 0
#define LV_USE_SYSMON           0
#define LV_USE_PROFILER         0
#define LV_USE_MONKEY           0

#define LV_USE_FLOAT            0
#define LV_USE_MATRIX           0
#define LV_USE_OBJ_ID           0
#define LV_USE_OBJ_NAME         0
#define LV_USE_USER_DATA        1
#define LV_ENABLE_GLOBAL_CUSTOM 0

/*==================
   FONT USAGE
 *==================*/

/* Apps ship their own converted fonts. Montserrat 14 is kept only as the
 * fallback behind una_lvgl_default_font() below and is dropped from the link
 * when the app provides that function. */
#define LV_FONT_MONTSERRAT_8    0
#define LV_FONT_MONTSERRAT_10   0
#define LV_FONT_MONTSERRAT_12   0
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_16   0
#define LV_FONT_MONTSERRAT_18   0
#define LV_FONT_MONTSERRAT_20   0
#define LV_FONT_MONTSERRAT_22   0
#define LV_FONT_MONTSERRAT_24   0
#define LV_FONT_MONTSERRAT_26   0
#define LV_FONT_MONTSERRAT_28   0
#define LV_FONT_MONTSERRAT_30   0
#define LV_FONT_MONTSERRAT_32   0
#define LV_FONT_MONTSERRAT_34   0
#define LV_FONT_MONTSERRAT_36   0
#define LV_FONT_MONTSERRAT_38   0
#define LV_FONT_MONTSERRAT_40   0
#define LV_FONT_MONTSERRAT_42   0
#define LV_FONT_MONTSERRAT_44   0
#define LV_FONT_MONTSERRAT_46   0
#define LV_FONT_MONTSERRAT_48   0
#define LV_FONT_MONTSERRAT_28_COMPRESSED 0
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0
#define LV_FONT_SIMSUN_14_CJK   0
#define LV_FONT_SIMSUN_16_CJK   0
#define LV_FONT_UNSCII_8        0
#define LV_FONT_UNSCII_16       0

/* The default font is whatever the app returns from una_lvgl_default_font():
 * one of its own converted faces, so no built-in font has to be linked. The
 * port supplies a weak fallback returning Montserrat 14. LVGL evaluates
 * LV_FONT_DEFAULT at run time (lv_font_default(), style defaults), so a call
 * is a valid definition. */
#define LV_FONT_CUSTOM_DECLARE  const lv_font_t* una_lvgl_default_font(void);
#define LV_FONT_DEFAULT         una_lvgl_default_font()
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
   WIDGETS
 *==================*/

#define LV_WIDGETS_HAS_DEFAULT_VALUE 1

#define LV_USE_ANIMIMG      0
#define LV_USE_ARC          1
#define LV_USE_BAR          1
#define LV_USE_BUTTON       1
#define LV_USE_BUTTONMATRIX 0
#define LV_USE_CALENDAR     0
#define LV_USE_CANVAS       0
#define LV_USE_CHART        0
#define LV_USE_CHECKBOX     0
#define LV_USE_DROPDOWN     0
#define LV_USE_IMAGE        1
#define LV_USE_IMAGEBUTTON  0
#define LV_USE_KEYBOARD     0
#define LV_USE_LABEL        1
#if LV_USE_LABEL
    #define LV_LABEL_TEXT_SELECTION  0
    #define LV_LABEL_LONG_TXT_HINT   1
    #define LV_LABEL_WAIT_CHAR_COUNT 3
#endif
#define LV_USE_LED          0
#define LV_USE_LINE         1
#define LV_USE_LIST         0
#define LV_USE_LOTTIE       0
#define LV_USE_MENU         0
#define LV_USE_MSGBOX       0
#define LV_USE_ROLLER       1
#define LV_USE_SCALE        0
#define LV_USE_SLIDER       0
#define LV_USE_SPAN         0
#define LV_USE_SPINBOX      0
#define LV_USE_SPINNER      0
#define LV_USE_SWITCH       1
#define LV_USE_TABLE        0
#define LV_USE_TABVIEW      0
#define LV_USE_TEXTAREA     0
#define LV_USE_TILEVIEW     0
#define LV_USE_WIN          0
#define LV_USE_3DTEXTURE    0

/*==================
   THEMES
 *==================*/

/* The app builds its own styles; no stock theme is compiled in. */
#define LV_USE_THEME_DEFAULT 0
#define LV_USE_THEME_SIMPLE  0
#define LV_USE_THEME_MONO    0

/*==================
   LAYOUTS
 *==================*/

#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/*====================
   3RD PARTY LIBRARIES
 *====================*/

#define LV_USE_FS_STDIO     0
#define LV_USE_FS_POSIX     0
#define LV_USE_FS_WIN32     0
#define LV_USE_FS_FATFS     0
#define LV_USE_FS_MEMFS     0
#define LV_USE_FS_LITTLEFS  0
#define LV_USE_FS_ARDUINO_ESP_LITTLEFS 0
#define LV_USE_FS_ARDUINO_SD 0
#define LV_USE_FS_UEFI      0
#define LV_USE_FS_FROGFS    0

#define LV_USE_LODEPNG      0
#define LV_USE_LIBPNG       0
#define LV_USE_BMP          0
#define LV_USE_TJPGD        0
#define LV_USE_LIBJPEG_TURBO 0
#define LV_USE_GIF          0
#define LV_BIN_DECODER_RAM_LOAD 0
#define LV_USE_RLE          0
#define LV_USE_QRCODE       0
#define LV_USE_BARCODE      0
#define LV_USE_FREETYPE     0
#define LV_USE_TINY_TTF     0
#define LV_USE_RLOTTIE      0
#define LV_USE_VECTOR_GRAPHIC 0
#define LV_USE_THORVG_INTERNAL 0
#define LV_USE_THORVG_EXTERNAL 0
#define LV_USE_LZ4_INTERNAL 0
#define LV_USE_LZ4_EXTERNAL 0
#define LV_USE_SVG          0
#define LV_USE_FFMPEG       0

/*==================
   OTHERS
 *==================*/

#define LV_USE_SNAPSHOT     0
#define LV_USE_FRAGMENT     0
#define LV_USE_IMGFONT      0
#define LV_USE_OBSERVER     1
#define LV_USE_IME_PINYIN   0
#define LV_USE_FILE_EXPLORER 0
#define LV_USE_FONT_MANAGER 0
#define LV_USE_TEST         0
#define LV_USE_XML          0
#define LV_USE_GRIDNAV      0

/*==================
   DEVICES
 *==================*/

#define LV_USE_SDL          0
#define LV_USE_X11          0
#define LV_USE_WAYLAND      0
#define LV_USE_LINUX_FBDEV  0
#define LV_USE_NUTTX        0
#define LV_USE_LINUX_DRM    0
#define LV_USE_TFT_ESPI     0
#define LV_USE_EVDEV        0
#define LV_USE_LIBINPUT     0
#define LV_USE_ST7735       0
#define LV_USE_ST7789       0
#define LV_USE_ST7796       0
#define LV_USE_ILI9341      0
#define LV_USE_FT81X        0
#define LV_USE_GENERIC_MIPI 0
#define LV_USE_RENESAS_GLCDC 0
#define LV_USE_ST_LTDC      0
#define LV_USE_WINDOWS      0
#define LV_USE_UEFI         0
#define LV_USE_OPENGLES     0
#define LV_USE_QNX          0

/*==================
   EXAMPLES / DEMOS
 *==================*/

#define LV_BUILD_EXAMPLES   0
#define LV_BUILD_DEMOS      0

#endif /* LV_CONF_H */
