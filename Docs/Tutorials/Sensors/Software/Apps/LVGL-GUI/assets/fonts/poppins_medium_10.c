/*******************************************************************************
 * Size: 10 px
 * Bpp: 2
 * Opts: --font Docs/Tutorials/Sensors/Software/Apps/TouchGFX-GUI/assets/fonts/Poppins-Medium.ttf --size 10 --bpp 2 --format lvgl --no-compress -r 0x20-0x7E -o Docs/Tutorials/Sensors/Software/Apps/LVGL-GUI/assets/fonts/poppins_medium_10.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef POPPINS_MEDIUM_10
#define POPPINS_MEDIUM_10 1
#endif

#if POPPINS_MEDIUM_10

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */

    /* U+0021 "!" */
    0x30, 0xc3, 0xc, 0x30, 0x83, 0x0,

    /* U+0022 "\"" */
    0x2, 0x79, 0xc0,

    /* U+0023 "#" */
    0x3, 0xc, 0x0, 0xc3, 0x3, 0xff, 0xf0, 0x24,
    0xa0, 0xd, 0x34, 0x2f, 0xff, 0x41, 0xc7, 0x0,
    0x61, 0x80,

    /* U+0024 "$" */
    0x0, 0x0, 0xa0, 0x3f, 0xc7, 0x54, 0x39, 0x1,
    0xf8, 0x5, 0xd7, 0x5d, 0x2f, 0x80, 0x50,

    /* U+0025 "%" */
    0x3c, 0x30, 0x99, 0x60, 0x28, 0xc0, 0x2, 0x80,
    0x3, 0x28, 0x9, 0x55, 0x1c, 0x28,

    /* U+0026 "&" */
    0xf, 0x80, 0x18, 0x80, 0x1c, 0x0, 0x37, 0x5c,
    0xa0, 0xf0, 0xb0, 0xf4, 0x2f, 0x9c,

    /* U+0027 "'" */
    0x9, 0x90,

    /* U+0028 "(" */
    0xd, 0x28, 0x30, 0x60, 0xa0, 0x90, 0x90, 0x90,
    0x60, 0x30, 0x34, 0xc,

    /* U+0029 ")" */
    0x70, 0x28, 0xc, 0xd, 0xa, 0x6, 0x6, 0xa,
    0xd, 0xc, 0x28, 0x70,

    /* U+002A "*" */
    0xc, 0xb, 0x83, 0xe0, 0x30, 0x0, 0x0,

    /* U+002B "+" */
    0x4, 0x0, 0xc0, 0xc, 0xf, 0xfc, 0xc, 0x0,
    0xc0,

    /* U+002C "," */
    0x79, 0xc0,

    /* U+002D "-" */
    0x3f, 0xc0, 0x0,

    /* U+002E "." */
    0x17,

    /* U+002F "/" */
    0x3, 0x40, 0xc0, 0x70, 0x18, 0x9, 0x3, 0x0,
    0xc0, 0x70, 0x28, 0xd, 0x3, 0x1, 0xc0,

    /* U+0030 "0" */
    0x1f, 0x83, 0x4d, 0x30, 0x67, 0x7, 0x70, 0x73,
    0x6, 0x34, 0xd1, 0xf8,

    /* U+0031 "1" */
    0x3c, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc,

    /* U+0032 "2" */
    0x1f, 0x83, 0x4d, 0x20, 0x90, 0xd, 0x2, 0xc0,
    0x70, 0x1c, 0x3, 0xfe,

    /* U+0033 "3" */
    0x1f, 0x83, 0x4d, 0x0, 0xd0, 0x78, 0x0, 0xd1,
    0xa, 0x34, 0xd1, 0xf8,

    /* U+0034 "4" */
    0x2, 0xc0, 0xf, 0x0, 0xdc, 0x6, 0x30, 0x30,
    0xc2, 0xff, 0xc0, 0xd, 0x0, 0x30,

    /* U+0035 "5" */
    0x3f, 0xd3, 0x0, 0x30, 0x3, 0xb8, 0x34, 0xa0,
    0x7, 0x34, 0xa1, 0xf8,

    /* U+0036 "6" */
    0xf, 0x83, 0x49, 0x30, 0x3, 0xb8, 0x34, 0xa3,
    0x7, 0x34, 0xa0, 0xfc,

    /* U+0037 "7" */
    0x3f, 0xe0, 0x9, 0x0, 0xc0, 0x18, 0x3, 0x0,
    0x70, 0x9, 0x0, 0xc0,

    /* U+0038 "8" */
    0x1f, 0x83, 0xa, 0x30, 0x92, 0xfc, 0x30, 0xa7,
    0x7, 0x30, 0xa1, 0xf8,

    /* U+0039 "9" */
    0x1f, 0x83, 0x49, 0x30, 0x63, 0xa, 0x1f, 0xe0,
    0x6, 0x34, 0x90, 0xf8,

    /* U+003A ":" */
    0x61, 0x0, 0x17,

    /* U+003B ";" */
    0x30, 0x40, 0x0, 0x0, 0xc6, 0x20,

    /* U+003C "<" */
    0x3, 0x43, 0x43, 0x80, 0x34, 0x3, 0x40,

    /* U+003D "=" */
    0xbf, 0xd0, 0x0, 0xbf, 0xd0, 0x0,

    /* U+003E ">" */
    0x70, 0x2c, 0xe, 0x1c, 0x70,

    /* U+003F "?" */
    0x2f, 0x18, 0x70, 0x1c, 0x7c, 0x14, 0x4, 0x2,
    0x80,

    /* U+0040 "@" */
    0x1, 0xbe, 0x0, 0xa0, 0x1c, 0x18, 0xfd, 0x93,
    0x34, 0xd6, 0x23, 0xc, 0x56, 0x30, 0xcc, 0x32,
    0xeb, 0x42, 0x80, 0x0, 0xb, 0xf0, 0x0,

    /* U+0041 "A" */
    0x7, 0x40, 0x2a, 0x0, 0xcc, 0x6, 0x24, 0x3f,
    0xf0, 0xc0, 0xc6, 0x2, 0x40,

    /* U+0042 "B" */
    0x7f, 0x87, 0xa, 0x70, 0x97, 0xfc, 0x70, 0xa7,
    0xa, 0x7f, 0xc0,

    /* U+0043 "C" */
    0xb, 0xe0, 0x38, 0x2c, 0x70, 0x0, 0xa0, 0x0,
    0x70, 0x0, 0x38, 0x2c, 0xb, 0xe0,

    /* U+0044 "D" */
    0x7f, 0x81, 0xc1, 0xc7, 0x2, 0x9c, 0x6, 0x70,
    0x29, 0xc1, 0xc7, 0xf8, 0x0,

    /* U+0045 "E" */
    0x7f, 0x9c, 0x7, 0x1, 0xfd, 0x70, 0x1c, 0x7,
    0xf8,

    /* U+0046 "F" */
    0x7f, 0xdc, 0x7, 0x1, 0xfc, 0x70, 0x1c, 0x7,
    0x0,

    /* U+0047 "G" */
    0xb, 0xe0, 0x38, 0x2c, 0x70, 0x0, 0xa1, 0xfd,
    0x70, 0xc, 0x38, 0x28, 0xb, 0xe0,

    /* U+0048 "H" */
    0x70, 0x35, 0xc0, 0xd7, 0x3, 0x5f, 0xfd, 0x70,
    0x35, 0xc0, 0xd7, 0x3, 0x40,

    /* U+0049 "I" */
    0x77, 0x77, 0x77, 0x70,

    /* U+004A "J" */
    0x2, 0x80, 0xa0, 0x28, 0xa, 0x2, 0x9c, 0xd2,
    0xe0,

    /* U+004B "K" */
    0x70, 0xe1, 0xca, 0x7, 0xb0, 0x1f, 0x0, 0x7b,
    0x1, 0xca, 0x7, 0xe, 0x0,

    /* U+004C "L" */
    0x70, 0x1c, 0x7, 0x1, 0xc0, 0x70, 0x1c, 0x7,
    0xf0,

    /* U+004D "M" */
    0x70, 0x7, 0x1e, 0x3, 0xc7, 0xc1, 0xf1, 0xe8,
    0x9c, 0x73, 0x33, 0x1c, 0xb4, 0xc7, 0xc, 0x30,

    /* U+004E "N" */
    0x70, 0x25, 0xf0, 0x97, 0xa2, 0x5c, 0xc9, 0x71,
    0xe5, 0xc2, 0xd7, 0x3, 0x40,

    /* U+004F "O" */
    0xb, 0xe0, 0x38, 0x2c, 0x70, 0xd, 0xa0, 0x9,
    0x70, 0xd, 0x38, 0x2c, 0xb, 0xe0,

    /* U+0050 "P" */
    0x7f, 0x87, 0xe, 0x70, 0xd7, 0xf8, 0x70, 0x7,
    0x0, 0x70, 0x0,

    /* U+0051 "Q" */
    0xb, 0xe0, 0x38, 0x2c, 0x70, 0xd, 0xa0, 0x9,
    0x70, 0xd, 0x38, 0x2c, 0xb, 0xf0, 0x0, 0x2c,
    0x0, 0x4,

    /* U+0052 "R" */
    0x7f, 0x87, 0xe, 0x70, 0xd7, 0xf8, 0x73, 0x47,
    0x1c, 0x70, 0xd0,

    /* U+0053 "S" */
    0x2f, 0x87, 0xc, 0x70, 0x1, 0xf4, 0x0, 0xd7,
    0xd, 0x2f, 0x80,

    /* U+0054 "T" */
    0xbf, 0xc0, 0xd0, 0xd, 0x0, 0xd0, 0xd, 0x0,
    0xd0, 0xd, 0x0,

    /* U+0055 "U" */
    0x70, 0x31, 0xc0, 0xc7, 0x3, 0x1c, 0xc, 0x30,
    0x30, 0xd1, 0xc1, 0xfc, 0x0,

    /* U+0056 "V" */
    0xa0, 0x29, 0xc0, 0xc3, 0x47, 0xa, 0x24, 0xc,
    0xc0, 0x3e, 0x0, 0x74, 0x0,

    /* U+0057 "W" */
    0xa0, 0xb0, 0xa7, 0xf, 0xd, 0x31, 0xf4, 0xc3,
    0x66, 0x9c, 0x2b, 0xe, 0x41, 0xf0, 0xf0, 0xe,
    0xb, 0x0,

    /* U+0058 "X" */
    0x30, 0x70, 0xa3, 0x0, 0xf8, 0x1, 0xc0, 0xf,
    0x80, 0xa3, 0x3, 0x7, 0x0,

    /* U+0059 "Y" */
    0xa0, 0xa3, 0xc, 0x2a, 0x80, 0xf0, 0xa, 0x0,
    0xa0, 0xa, 0x0,

    /* U+005A "Z" */
    0xbf, 0xc0, 0x28, 0x3, 0x0, 0xd0, 0x2c, 0x3,
    0x0, 0x7f, 0xc0,

    /* U+005B "[" */
    0xba, 0x8a, 0x28, 0xa2, 0x8a, 0x28, 0xa2, 0x8a,
    0x2e, 0x0,

    /* U+005C "\\" */
    0x30, 0xc, 0x2, 0x40, 0x60, 0xc, 0x3, 0x0,
    0x90, 0x18, 0x3, 0x0, 0xc0, 0x24, 0x6,

    /* U+005D "]" */
    0xb4, 0x92, 0x49, 0x24, 0x92, 0x49, 0x24, 0x92,
    0x6d, 0x0,

    /* U+005E "^" */
    0x1, 0x0, 0x2c, 0x0, 0xe8, 0xa, 0x30, 0x30,
    0xa1, 0xc0, 0xc0,

    /* U+005F "_" */
    0xbf, 0xf0, 0x0,

    /* U+0060 "`" */
    0x2, 0x80, 0x0,

    /* U+0061 "a" */
    0x1f, 0x71, 0xd3, 0xca, 0x3, 0x28, 0xc, 0x70,
    0xb0, 0x7e, 0xc0,

    /* U+0062 "b" */
    0x70, 0x1, 0xc0, 0x7, 0xbd, 0x1e, 0x1c, 0x70,
    0x35, 0xc0, 0xd7, 0x87, 0x1e, 0xf4,

    /* U+0063 "c" */
    0x1f, 0x87, 0xd, 0xa0, 0xa, 0x0, 0x70, 0xd1,
    0xf8,

    /* U+0064 "d" */
    0x0, 0x30, 0x0, 0xc1, 0xf7, 0x1d, 0x3c, 0xa0,
    0x32, 0x80, 0xc7, 0xb, 0x7, 0xec,

    /* U+0065 "e" */
    0x1f, 0x87, 0xa, 0xbf, 0xfa, 0x0, 0x74, 0xd1,
    0xf8,

    /* U+0066 "f" */
    0x2c, 0x30, 0xfc, 0x30, 0x30, 0x30, 0x30, 0x30,

    /* U+0067 "g" */
    0x1f, 0xb1, 0xc2, 0xca, 0x3, 0x28, 0xc, 0x74,
    0xf0, 0x7d, 0xc0, 0x3, 0xd, 0x28, 0x1f, 0x80,

    /* U+0068 "h" */
    0x70, 0x7, 0x0, 0x7b, 0xc7, 0x4e, 0x70, 0x77,
    0x7, 0x70, 0x77, 0x7,

    /* U+0069 "i" */
    0x70, 0x47, 0x1c, 0x71, 0xc7, 0x1c,

    /* U+006A "j" */
    0x1c, 0x4, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c,
    0x1c, 0x1c, 0x74,

    /* U+006B "k" */
    0x70, 0x7, 0x0, 0x71, 0xc7, 0x70, 0x7d, 0x7,
    0xd0, 0x77, 0x7, 0x1c,

    /* U+006C "l" */
    0x77, 0x77, 0x77, 0x77,

    /* U+006D "m" */
    0x7b, 0xdf, 0x87, 0x4b, 0xe, 0x70, 0x70, 0x67,
    0x7, 0x6, 0x70, 0x70, 0x67, 0x7, 0x6,

    /* U+006E "n" */
    0x7b, 0xc7, 0x4a, 0x70, 0x77, 0x7, 0x70, 0x77,
    0x7,

    /* U+006F "o" */
    0x1f, 0x87, 0x4e, 0xa0, 0x7a, 0x7, 0x74, 0xe1,
    0xf8,

    /* U+0070 "p" */
    0x7b, 0xd1, 0xe1, 0xc7, 0x3, 0x5c, 0xd, 0x78,
    0x71, 0xef, 0x47, 0x0, 0x1c, 0x0, 0x70, 0x0,

    /* U+0071 "q" */
    0x1f, 0x71, 0xd2, 0xca, 0x3, 0x28, 0xc, 0x74,
    0xb0, 0x7d, 0xc0, 0x3, 0x0, 0xc, 0x0, 0x30,

    /* U+0072 "r" */
    0x7a, 0x78, 0x70, 0x70, 0x70, 0x70,

    /* U+0073 "s" */
    0x2f, 0x1c, 0xa3, 0x90, 0x1e, 0x61, 0xcb, 0xd0,

    /* U+0074 "t" */
    0x30, 0xbd, 0x30, 0x30, 0x30, 0x30, 0x2d,

    /* U+0075 "u" */
    0x70, 0x67, 0x6, 0x70, 0x67, 0x6, 0x30, 0xe1,
    0xfa,

    /* U+0076 "v" */
    0x90, 0x97, 0xc, 0x31, 0xc2, 0x64, 0x1f, 0x0,
    0xe0,

    /* U+0077 "w" */
    0xd2, 0xc3, 0x18, 0xf1, 0x83, 0x39, 0x90, 0xd9,
    0xb0, 0x2d, 0x3c, 0x7, 0xe, 0x0,

    /* U+0078 "x" */
    0xa2, 0x8d, 0xc1, 0xd0, 0x74, 0x37, 0x28, 0xa0,

    /* U+0079 "y" */
    0xa0, 0xa7, 0xc, 0x31, 0xc2, 0xa4, 0x1f, 0x0,
    0xe0, 0xd, 0x1, 0xc0, 0x28, 0x0,

    /* U+007A "z" */
    0xbf, 0x41, 0xc0, 0xd0, 0xa0, 0x70, 0x2f, 0xd0,

    /* U+007B "{" */
    0x1c, 0x34, 0x30, 0x34, 0x34, 0x70, 0xb0, 0x34,
    0x34, 0x30, 0x34, 0x2c, 0x0,

    /* U+007C "|" */
    0xcc, 0xcc, 0xcc, 0xcc, 0xcc,

    /* U+007D "}" */
    0x38, 0x1c, 0xc, 0xc, 0xc, 0xe, 0xa, 0xc,
    0xc, 0xc, 0xc, 0x38, 0x0,

    /* U+007E "~" */
    0x38, 0xc5, 0xb8, 0x0, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 42, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 0, .adv_w = 51, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 6, .adv_w = 52, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 9, .adv_w = 140, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 27, .adv_w = 104, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 42, .adv_w = 127, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 56, .adv_w = 122, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 70, .adv_w = 28, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 72, .adv_w = 79, .box_w = 4, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 84, .adv_w = 79, .box_w = 4, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 96, .adv_w = 81, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 103, .adv_w = 114, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 112, .adv_w = 36, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 114, .adv_w = 94, .box_w = 5, .box_h = 2, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 117, .adv_w = 39, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 118, .adv_w = 82, .box_w = 5, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 133, .adv_w = 104, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 145, .adv_w = 104, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 153, .adv_w = 104, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 165, .adv_w = 104, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 177, .adv_w = 104, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 191, .adv_w = 104, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 203, .adv_w = 104, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 215, .adv_w = 104, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 227, .adv_w = 104, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 239, .adv_w = 104, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 251, .adv_w = 39, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 254, .adv_w = 49, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 260, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 267, .adv_w = 124, .box_w = 6, .box_h = 4, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 273, .adv_w = 94, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 278, .adv_w = 86, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 287, .adv_w = 164, .box_w = 10, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 310, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 323, .adv_w = 101, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 334, .adv_w = 124, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 348, .adv_w = 113, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 361, .adv_w = 84, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 370, .adv_w = 82, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 379, .adv_w = 124, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 393, .adv_w = 113, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 406, .adv_w = 42, .box_w = 2, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 410, .adv_w = 90, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 419, .adv_w = 101, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 432, .adv_w = 71, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 441, .adv_w = 141, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 457, .adv_w = 115, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 470, .adv_w = 125, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 484, .adv_w = 95, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 495, .adv_w = 126, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 513, .adv_w = 101, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 524, .adv_w = 97, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 535, .adv_w = 90, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 546, .adv_w = 110, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 559, .adv_w = 111, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 572, .adv_w = 160, .box_w = 10, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 590, .adv_w = 106, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 603, .adv_w = 97, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 614, .adv_w = 89, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 625, .adv_w = 78, .box_w = 3, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 635, .adv_w = 115, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 650, .adv_w = 77, .box_w = 3, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 660, .adv_w = 105, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 671, .adv_w = 128, .box_w = 6, .box_h = 2, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 674, .adv_w = 41, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 677, .adv_w = 108, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 688, .adv_w = 108, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 702, .adv_w = 96, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 711, .adv_w = 108, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 725, .adv_w = 99, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 734, .adv_w = 53, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 742, .adv_w = 108, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 758, .adv_w = 104, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 770, .adv_w = 42, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 776, .adv_w = 42, .box_w = 4, .box_h = 11, .ofs_x = -1, .ofs_y = -3},
    {.bitmap_index = 787, .adv_w = 89, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 799, .adv_w = 42, .box_w = 2, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 803, .adv_w = 166, .box_w = 10, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 818, .adv_w = 104, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 827, .adv_w = 102, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 836, .adv_w = 108, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 852, .adv_w = 108, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 868, .adv_w = 61, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 874, .adv_w = 85, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 882, .adv_w = 60, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 889, .adv_w = 104, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 898, .adv_w = 92, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 907, .adv_w = 132, .box_w = 9, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 921, .adv_w = 80, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 929, .adv_w = 93, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 943, .adv_w = 75, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 951, .adv_w = 83, .box_w = 4, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 964, .adv_w = 52, .box_w = 2, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 969, .adv_w = 83, .box_w = 4, .box_h = 13, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 982, .adv_w = 88, .box_w = 6, .box_h = 3, .ofs_x = 0, .ofs_y = 2}
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
    .cmap_num = 1,
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
const lv_font_t poppins_medium_10 = {
#else
lv_font_t poppins_medium_10 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 13,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if POPPINS_MEDIUM_10*/

