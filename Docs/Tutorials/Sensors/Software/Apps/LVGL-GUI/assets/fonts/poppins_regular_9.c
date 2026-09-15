/*******************************************************************************
 * Size: 9 px
 * Bpp: 2
 * Opts: --font Docs/Tutorials/Sensors/Software/Apps/TouchGFX-GUI/assets/fonts/Poppins-Regular.ttf --size 9 --bpp 2 --format lvgl --no-compress -r 0x20-0x7E,0xB0 -o Docs/Tutorials/Sensors/Software/Apps/LVGL-GUI/assets/fonts/poppins_regular_9.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef POPPINS_REGULAR_9
#define POPPINS_REGULAR_9 1
#endif

#if POPPINS_REGULAR_9

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */

    /* U+0021 "!" */
    0x22, 0x22, 0x12,

    /* U+0022 "\"" */
    0x92, 0x40,

    /* U+0023 "#" */
    0x5, 0x20, 0x8, 0x30, 0x2e, 0xf8, 0xc, 0x50,
    0xbe, 0xf4, 0x14, 0x80, 0x20, 0xc0,

    /* U+0024 "$" */
    0x4, 0xb, 0xc5, 0x44, 0xe0, 0xa, 0x45, 0x22,
    0xe0, 0x0,

    /* U+0025 "%" */
    0x68, 0x52, 0x22, 0x2, 0x60, 0x1, 0x98, 0x8,
    0x94, 0x92, 0x80,

    /* U+0026 "&" */
    0x1b, 0x0, 0x80, 0x2, 0x81, 0x24, 0x98, 0x90,
    0xd0, 0xa9, 0xc0,

    /* U+0027 "'" */
    0x88,

    /* U+0028 "(" */
    0x5, 0x8, 0x20, 0x30, 0x30, 0x30, 0x30, 0x20,
    0x8, 0x9,

    /* U+0029 ")" */
    0x60, 0x20, 0x8, 0xc, 0x8, 0x8, 0xc, 0x8,
    0x24, 0x20,

    /* U+002A "*" */
    0x4, 0x2d, 0x19, 0x0,

    /* U+002B "+" */
    0x1, 0x0, 0x50, 0x2f, 0x90, 0x50, 0x5, 0x0,

    /* U+002C "," */
    0x0, 0x82, 0x0,

    /* U+002D "-" */
    0x6a, 0x0,

    /* U+002E "." */
    0x5,

    /* U+002F "/" */
    0x2, 0x5, 0x8, 0xc, 0x8, 0x14, 0x20, 0x30,
    0x20, 0x50,

    /* U+0030 "0" */
    0x1b, 0x2, 0xc, 0x50, 0xc5, 0x8, 0x50, 0xc2,
    0xc, 0x1f, 0x0,

    /* U+0031 "1" */
    0x34, 0x51, 0x45, 0x14, 0x51, 0x40,

    /* U+0032 "2" */
    0x1a, 0x2, 0xc, 0x0, 0xc0, 0x14, 0x6, 0x1,
    0x80, 0x3a, 0x80,

    /* U+0033 "3" */
    0x1a, 0x8, 0x20, 0x8, 0x2c, 0x0, 0x88, 0x32,
    0xa4,

    /* U+0034 "4" */
    0x3, 0x80, 0xa8, 0x19, 0x82, 0x18, 0xbb, 0xd0,
    0x18,

    /* U+0035 "5" */
    0x3a, 0x82, 0x0, 0x2a, 0x42, 0xc, 0x0, 0x82,
    0xc, 0x1a, 0x40,

    /* U+0036 "6" */
    0x1a, 0x2, 0x8, 0x50, 0x6, 0xa4, 0x60, 0xc2,
    0x8, 0x1a, 0x40,

    /* U+0037 "7" */
    0x2a, 0xc0, 0x20, 0x20, 0xc, 0x5, 0x2, 0x0,
    0x80,

    /* U+0038 "8" */
    0x2a, 0x42, 0xc, 0x20, 0xc2, 0xf4, 0x60, 0xc5,
    0xc, 0x2a, 0x40,

    /* U+0039 "9" */
    0x1a, 0x2, 0x8, 0x50, 0x82, 0xc, 0x1a, 0x81,
    0x8, 0x1a, 0x40,

    /* U+003A ":" */
    0x50, 0x0, 0x50,

    /* U+003B ";" */
    0x60, 0x0, 0x58, 0x0,

    /* U+003C "<" */
    0x0, 0x9, 0x24, 0x24, 0x9,

    /* U+003D "=" */
    0x2a, 0x90, 0x0, 0x2a, 0x90,

    /* U+003E ">" */
    0x0, 0x6, 0x0, 0x60, 0x24, 0x24, 0x0,

    /* U+003F "?" */
    0x2a, 0x10, 0x80, 0x20, 0xa4, 0x10, 0x8, 0x0,

    /* U+0040 "@" */
    0x1, 0xa8, 0x2, 0x0, 0x82, 0x1a, 0x54, 0x98,
    0x55, 0x59, 0x21, 0x14, 0xea, 0x42, 0x0, 0x0,
    0x2a, 0x40,

    /* U+0041 "A" */
    0xa, 0x0, 0xf0, 0x15, 0x43, 0xc, 0x3a, 0xc9,
    0x6,

    /* U+0042 "B" */
    0x7a, 0x46, 0xc, 0x7b, 0x46, 0xc, 0x60, 0xc7,
    0xa4,

    /* U+0043 "C" */
    0x1a, 0x90, 0xc0, 0x89, 0x0, 0x24, 0x0, 0x30,
    0x20, 0x6a, 0x40,

    /* U+0044 "D" */
    0x7a, 0x86, 0x6, 0x60, 0x36, 0x3, 0x60, 0x67,
    0xa8,

    /* U+0045 "E" */
    0x7a, 0x18, 0x7, 0xa1, 0x80, 0x60, 0x1e, 0x80,

    /* U+0046 "F" */
    0x7a, 0x18, 0x7, 0x91, 0x80, 0x60, 0x18, 0x0,

    /* U+0047 "G" */
    0x1a, 0xd0, 0xc0, 0x49, 0x2a, 0xa4, 0x5, 0x30,
    0x30, 0x6a, 0x0,

    /* U+0048 "H" */
    0x60, 0x66, 0x6, 0x7a, 0xe6, 0x6, 0x60, 0x66,
    0x6,

    /* U+0049 "I" */
    0x66, 0x66, 0x66,

    /* U+004A "J" */
    0x3, 0x3, 0x3, 0x3, 0x43, 0x3d,

    /* U+004B "K" */
    0x61, 0x86, 0x60, 0x68, 0x7, 0x80, 0x66, 0x6,
    0x18,

    /* U+004C "L" */
    0x60, 0x60, 0x60, 0x60, 0x60, 0x79,

    /* U+004D "M" */
    0x70, 0xc, 0x74, 0x2c, 0x68, 0x2c, 0x68, 0x5c,
    0x62, 0x8c, 0x62, 0x4c,

    /* U+004E "N" */
    0x70, 0x27, 0x82, 0x69, 0x26, 0x22, 0x60, 0xe6,
    0xa,

    /* U+004F "O" */
    0x1a, 0x90, 0xc0, 0x99, 0x0, 0xa4, 0x2, 0x30,
    0x24, 0x6a, 0x40,

    /* U+0050 "P" */
    0x7a, 0x58, 0x36, 0xd, 0xe8, 0x60, 0x18, 0x0,

    /* U+0051 "Q" */
    0x1a, 0x90, 0xc0, 0x99, 0x0, 0xa4, 0x2, 0x30,
    0x24, 0x6b, 0x80, 0x2, 0x40,

    /* U+0052 "R" */
    0x7a, 0x58, 0x36, 0xd, 0xec, 0x63, 0x18, 0x60,

    /* U+0053 "S" */
    0x2b, 0x14, 0x13, 0x40, 0x19, 0x50, 0x8a, 0x80,

    /* U+0054 "T" */
    0x6e, 0x43, 0x0, 0xc0, 0x30, 0xc, 0x3, 0x0,

    /* U+0055 "U" */
    0x60, 0x56, 0x5, 0x60, 0x56, 0x5, 0x20, 0x92,
    0xa8,

    /* U+0056 "V" */
    0x90, 0x22, 0x9, 0x30, 0xc1, 0x54, 0xe, 0x0,
    0xa0,

    /* U+0057 "W" */
    0x80, 0xc1, 0x54, 0x74, 0x82, 0x22, 0x30, 0x88,
    0xd8, 0x19, 0x28, 0x3, 0x7, 0x0,

    /* U+0058 "X" */
    0x60, 0xc2, 0x60, 0xe, 0x0, 0xe0, 0x22, 0x6,
    0xc,

    /* U+0059 "Y" */
    0x90, 0xc3, 0x24, 0x16, 0x0, 0xc0, 0xc, 0x0,
    0xc0,

    /* U+005A "Z" */
    0x6b, 0x40, 0xc0, 0x80, 0x60, 0x30, 0x2e, 0x90,

    /* U+005B "[" */
    0xac, 0xcc, 0xcc, 0xcc, 0xca,

    /* U+005C "\\" */
    0x80, 0x50, 0x20, 0x30, 0x24, 0x18, 0xc, 0x8,
    0x5, 0x2,

    /* U+005D "]" */
    0x28, 0x20, 0x82, 0x8, 0x20, 0x82, 0x8, 0xa0,

    /* U+005E "^" */
    0x4, 0x0, 0xe0, 0x16, 0x3, 0x8, 0x50, 0x80,

    /* U+005F "_" */
    0x3f, 0xe0,

    /* U+0060 "`" */
    0x2, 0x1, 0x0,

    /* U+0061 "a" */
    0x2e, 0x96, 0xd, 0x90, 0x56, 0x9, 0x2a, 0x90,

    /* U+0062 "b" */
    0x60, 0x6, 0x0, 0x6b, 0x87, 0x6, 0x60, 0x27,
    0x5, 0x6b, 0x80,

    /* U+0063 "c" */
    0x2a, 0x45, 0x8, 0x90, 0x5, 0x8, 0x2b, 0x40,

    /* U+0064 "d" */
    0x0, 0x50, 0x5, 0x2e, 0x96, 0xd, 0x90, 0x56,
    0x9, 0x2a, 0x90,

    /* U+0065 "e" */
    0x2b, 0x45, 0xc, 0xaa, 0xc5, 0x4, 0x2b, 0x40,

    /* U+0066 "f" */
    0x24, 0x8b, 0x48, 0x20, 0x82, 0x0,

    /* U+0067 "g" */
    0x2a, 0x96, 0x9, 0x90, 0x56, 0xd, 0x2e, 0x91,
    0x9, 0x2e, 0x40,

    /* U+0068 "h" */
    0x60, 0x6, 0x0, 0x6b, 0x46, 0xc, 0x60, 0x86,
    0x8, 0x60, 0x80,

    /* U+0069 "i" */
    0x60, 0x66, 0x66, 0x60,

    /* U+006A "j" */
    0x18, 0x1, 0x86, 0x18, 0x61, 0x86, 0x34,

    /* U+006B "k" */
    0x60, 0x18, 0x6, 0x31, 0xb0, 0x74, 0x1b, 0x6,
    0x30,

    /* U+006C "l" */
    0x66, 0x66, 0x66, 0x60,

    /* U+006D "m" */
    0x6a, 0x6a, 0x18, 0x30, 0x66, 0xc, 0x9, 0x83,
    0x2, 0x60, 0xc0, 0x80,

    /* U+006E "n" */
    0x7a, 0x46, 0xc, 0x60, 0x86, 0x8, 0x60, 0x80,

    /* U+006F "o" */
    0x2f, 0x46, 0xc, 0x90, 0x96, 0xc, 0x2f, 0x40,

    /* U+0070 "p" */
    0x6b, 0x87, 0x6, 0x60, 0x27, 0x5, 0x6b, 0x86,
    0x0, 0x60, 0x0,

    /* U+0071 "q" */
    0x2e, 0x96, 0xd, 0x90, 0x96, 0xd, 0x2e, 0x90,
    0x5, 0x0, 0x50,

    /* U+0072 "r" */
    0x6c, 0x60, 0x60, 0x60, 0x60,

    /* U+0073 "s" */
    0x2a, 0x14, 0x42, 0x90, 0x8, 0x2a, 0x0,

    /* U+0074 "t" */
    0x22, 0xe2, 0x8, 0x20, 0xe0,

    /* U+0075 "u" */
    0x50, 0xc5, 0xc, 0x50, 0xc6, 0xc, 0x2a, 0xc0,

    /* U+0076 "v" */
    0x80, 0x98, 0x53, 0x30, 0xa8, 0xc, 0x0,

    /* U+0077 "w" */
    0x83, 0x48, 0x52, 0x88, 0x24, 0x84, 0x28, 0xa0,
    0x28, 0x70,

    /* U+0078 "x" */
    0x92, 0xe, 0x1, 0xc0, 0xa0, 0x93, 0x0,

    /* U+0079 "y" */
    0x80, 0x98, 0x93, 0x30, 0x64, 0xc, 0x2, 0x2,
    0x0,

    /* U+007A "z" */
    0x6e, 0x8, 0x18, 0x30, 0xb9,

    /* U+007B "{" */
    0x9, 0x18, 0x8, 0x8, 0x24, 0x24, 0x8, 0x8,
    0x18, 0x9,

    /* U+007C "|" */
    0x22, 0x22, 0x22, 0x22,

    /* U+007D "}" */
    0x60, 0x14, 0x14, 0x14, 0x8, 0x8, 0x14, 0x14,
    0x14, 0x60,

    /* U+007E "~" */
    0x25, 0x12, 0x80,

    /* U+00B0 "°" */
    0x24, 0x85, 0x85, 0x24
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 38, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 0, .adv_w = 43, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3, .adv_w = 42, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 5, .adv_w = 121, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 19, .adv_w = 90, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 29, .adv_w = 109, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 40, .adv_w = 106, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 51, .adv_w = 23, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 52, .adv_w = 65, .box_w = 4, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 62, .adv_w = 65, .box_w = 4, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 72, .adv_w = 70, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 76, .adv_w = 98, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 84, .adv_w = 29, .box_w = 3, .box_h = 3, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 87, .adv_w = 79, .box_w = 5, .box_h = 1, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 89, .adv_w = 30, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 90, .adv_w = 69, .box_w = 4, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 100, .adv_w = 91, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 111, .adv_w = 91, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 117, .adv_w = 91, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 128, .adv_w = 91, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 137, .adv_w = 91, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 146, .adv_w = 91, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 157, .adv_w = 91, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 168, .adv_w = 91, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 177, .adv_w = 91, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 188, .adv_w = 91, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 199, .adv_w = 31, .box_w = 2, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 202, .adv_w = 38, .box_w = 2, .box_h = 7, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 206, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 211, .adv_w = 104, .box_w = 6, .box_h = 3, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 216, .adv_w = 78, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 223, .adv_w = 75, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 231, .adv_w = 146, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 249, .adv_w = 97, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 258, .adv_w = 88, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 267, .adv_w = 111, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 278, .adv_w = 102, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 287, .adv_w = 74, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 295, .adv_w = 73, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 303, .adv_w = 112, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 314, .adv_w = 100, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 323, .adv_w = 35, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 326, .adv_w = 76, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 332, .adv_w = 86, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 341, .adv_w = 62, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 347, .adv_w = 124, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 359, .adv_w = 101, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 368, .adv_w = 113, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 379, .adv_w = 83, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 387, .adv_w = 113, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 400, .adv_w = 88, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 408, .adv_w = 85, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 416, .adv_w = 78, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 424, .adv_w = 97, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 433, .adv_w = 97, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 442, .adv_w = 141, .box_w = 9, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 456, .adv_w = 89, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 465, .adv_w = 84, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 474, .adv_w = 78, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 482, .adv_w = 61, .box_w = 2, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 487, .adv_w = 95, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 497, .adv_w = 61, .box_w = 3, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 505, .adv_w = 91, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 513, .adv_w = 106, .box_w = 6, .box_h = 1, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 515, .adv_w = 37, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 518, .adv_w = 97, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 526, .adv_w = 97, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 537, .adv_w = 87, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 545, .adv_w = 97, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 556, .adv_w = 89, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 564, .adv_w = 47, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 570, .adv_w = 97, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 581, .adv_w = 92, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 592, .adv_w = 35, .box_w = 2, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 596, .adv_w = 36, .box_w = 3, .box_h = 9, .ofs_x = -1, .ofs_y = -2},
    {.bitmap_index = 603, .adv_w = 74, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 612, .adv_w = 35, .box_w = 2, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 616, .adv_w = 148, .box_w = 9, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 628, .adv_w = 92, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 636, .adv_w = 92, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 644, .adv_w = 97, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 655, .adv_w = 97, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 666, .adv_w = 54, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 671, .adv_w = 75, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 678, .adv_w = 52, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 683, .adv_w = 92, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 691, .adv_w = 81, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 698, .adv_w = 118, .box_w = 8, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 708, .adv_w = 69, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 715, .adv_w = 81, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 724, .adv_w = 66, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 729, .adv_w = 67, .box_w = 4, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 739, .adv_w = 42, .box_w = 2, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 743, .adv_w = 67, .box_w = 4, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 753, .adv_w = 75, .box_w = 5, .box_h = 2, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 756, .adv_w = 59, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 3}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 176, .range_length = 1, .glyph_id_start = 96,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 2,
    .bpp = 2,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t poppins_regular_9 = {
#else
lv_font_t poppins_regular_9 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 10,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if POPPINS_REGULAR_9*/

