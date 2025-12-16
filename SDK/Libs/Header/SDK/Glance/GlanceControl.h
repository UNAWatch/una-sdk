/**
 ******************************************************************************
 * @file    GlanceControl.h
 * @date    18-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   DTO for Glance applications
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __GLANCE_CONTROL_H
#define __GLANCE_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define GLANCE_TEXT_SIZE    (32)

typedef enum {
    GLANCE_ALIGN_H_LEFT = 0,
    GLANCE_ALIGN_H_CENTER,
    GLANCE_ALIGN_H_RIGHT
} GlanceAlignH_t;

typedef enum {
    GLANCE_ALIGN_V_TOP = 0,
    GLANCE_ALIGN_V_CENTER,
    GLANCE_ALIGN_V_BOTTOM
} GlanceAlignV_t;

typedef enum {
    GLANCE_TYPE_TEXT = 0,
    GLANCE_TYPE_IMAGE,
    GLANCE_TYPE_LINE,
    GLANCE_TYPE_RECT,
    GLANCE_TYPES_COUNT
} GlanceType_t;

typedef enum {
    GLANCE_COLOR_WHITE        = 0x3F, // C0C0C0 -> 11 11 11
    GLANCE_COLOR_GRAY         = 0x2A, // 808080 -> 10 10 10
    GLANCE_COLOR_GRAY_DARK    = 0x15, // 404040 -> 01 01 01
    GLANCE_COLOR_BLACK        = 0x00, // 000000 -> 00 00 00
    GLANCE_COLOR_YELLOW_DARK  = 0x38, // C08000 -> 11 10 00
    GLANCE_COLOR_TEAL         = 0x0A, // 008080 -> 00 10 10
    GLANCE_COLOR_TEAL_DARK    = 0x05, // 004040 -> 00 01 01
    GLANCE_COLOR_GREEN        = 0x0C, // 00C000 -> 00 11 00
    GLANCE_COLOR_DARK_GREEN   = 0x04, // 004000 -> 00 01 00
    GLANCE_COLOR_BROWN        = 0x39, // C08040 -> 11 10 01
    GLANCE_COLOR_RED          = 0x30, // C00000 -> 11 00 00
    GLANCE_COLOR_DARK_RED     = 0x10, // 400000 -> 01 00 00
    GLANCE_COLOR_BLUE         = 0x03, // 0000C0 -> 00 00 11
    GLANCE_COLOR_DARK_BLUE    = 0x01, // 000040 -> 00 00 01
    GLANCE_COLOR_CYAN         = 0x1F, // 40C0C0 -> 01 11 11
    GLANCE_COLOR_CYAN_LIGHT   = 0x2F, // 80C0C0 -> 10 11 11
} GlanceColor_t;

typedef enum {
    GLANCE_FONT_POPPINS_REGULAR_18 = 0,
    GLANCE_FONT_POPPINS_MEDIUM_10,
    GLANCE_FONT_POPPINS_MEDIUM_18,
    GLANCE_FONT_POPPINS_MEDIUM_25,
    GLANCE_FONT_POPPINS_SEMIBOLD_18,
    GLANCE_FONT_POPPINS_SEMIBOLD_20,
    GLANCE_FONT_POPPINS_SEMIBOLD_25,
    GLANCE_FONT_POPPINS_SEMIBOLD_30,
    GLANCE_FONT_POPPINS_SEMIBOLD_35,
    GLANCE_FONT_POPPINS_ITALIC_18,
    GLANCE_FONT_POPPINS_ITALIC_20,
    GLANCE_FONTS_COUNT
} GlanceFont_t;

typedef struct {
    uint16_t x;
    uint16_t y;
} GlancePoint_t;

typedef struct {
    uint16_t w;
    uint16_t h;
} GlanceSize_t;

typedef struct {
    GlancePoint_t  pos;
    GlanceSize_t   size;
    GlanceFont_t   font;
    GlanceAlignH_t align;
    uint8_t        color;
    char           str[GLANCE_TEXT_SIZE + 1];
} GlanceText_t;

typedef struct {
    GlancePoint_t  pos;
    GlanceSize_t   size;
    const uint8_t* buff;
} GlanceImage_t;

typedef struct {
    GlancePoint_t start;
    GlancePoint_t stop;
    uint8_t       color;
} GlanceLine_t;

typedef struct {
    GlancePoint_t pos;
    GlanceSize_t  size;
    uint8_t       color;
    uint8_t       bgColor;
    bool          fill;
} GlanceRect_t;

typedef union {
    GlanceText_t  text;
    GlanceImage_t image;
    GlanceLine_t  line;
    GlanceRect_t  rect;
} GlancePayload_t;

typedef struct {
    uint32_t        id;
    bool            valid;
    GlanceType_t    type;
    GlancePayload_t payload;
} GlanceControl_t;

#ifdef __cplusplus
}
#endif

#endif
