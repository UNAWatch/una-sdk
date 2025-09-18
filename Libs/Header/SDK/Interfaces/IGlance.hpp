/**
 ******************************************************************************
 * @file    IGlance.hpp
 * @date    18-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   API for use in Glance applications
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __I_GLANCE_HPP
#define __I_GLANCE_HPP

#include <cstdint>
#include <cstdbool>

namespace SDK::Interface {

    class IGlance {
    public:
        virtual ~IGlance() = default;

        enum class AlighH {
            LEFT,
            CENTER,
            RIGHT
        };

        enum class AlighV { 
            TOP,
            CENTER,
            BOTTOM
        };

        enum Color {
            WHITE        = 0x3F, // C0C0C0 -> 11 11 11
            GRAY         = 0x2A, // 808080 -> 10 10 10
            GRAY_DARK    = 0x15, // 404040 -> 01 01 01
            BLACK        = 0x00, // 000000 -> 00 00 00
            YELLOW_DARK  = 0x38, // C08000 -> 11 10 00
            TEAL         = 0x0A, // 008080 -> 00 10 10
            TEAL_DARK    = 0x05, // 004040 -> 00 01 01
            GREEN        = 0x0C, // 00C000 -> 00 11 00
            DARK_GREEN   = 0x04, // 004000 -> 00 01 00
            BROWN        = 0x39, // C08040 -> 11 10 01
            RED          = 0x30, // C00000 -> 11 00 00
            DARK_RED     = 0x10, // 400000 -> 01 00 00
            BLUE         = 0x03, // 0000C0 -> 00 00 11
            DARK_BLUE    = 0x01, // 000040 -> 00 00 01
            CYAN         = 0x1F, // 40C0C0 -> 01 11 11
            CYAN_LIGHT   = 0x2F, // 80C0C0 -> 10 11 11
        };

        enum class Font {
            POPPINS_REGULAR_18 = 0,
            POPPINS_MEDIUM_10,
            POPPINS_MEDIUM_18,
            POPPINS_MEDIUM_25,
            POPPINS_MEDIUM_50,
            POPPINS_MEDIUM_60,
            POPPINS_SEMIBOLD_18,
            POPPINS_SEMIBOLD_20,
            POPPINS_SEMIBOLD_25,
            POPPINS_SEMIBOLD_30,
            POPPINS_SEMIBOLD_35,
            POPPINS_SEMIBOLD_40,
            POPPINS_SEMIBOLD_60,
            POPPINS_ITALIC_18,
            POPPINS_ITALIC_20,
            POPPINS_LIGHTITALIC_18,
            POPPINS_LIGHT_60,
        };

        class Point {
        public:
            Point()
                : x()
                , y()
            {}

            Point(uint16_t x, uint16_t y)
                : x(x)
                , y(y)
            {}

            uint16_t x;
            uint16_t y;
        };

        class Size {
        public:
            Size()
                : w()
                , h()
            {}

            Size(uint16_t w, uint16_t h)
                : w(w)
                , h(h)
            {}

            uint16_t w;
            uint16_t h;
        };

        class Region {
        public:
            Region()
                : p()
                , s()
                , alignH(AlighH::CENTER)
                , alignV(AlighV::CENTER)
                , fontID(Font::POPPINS_REGULAR_18)
            {}

            Region(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
                : p(x, y)
                , s(w, h)
                , alignH(AlighH::CENTER)
                , alignV(AlighV::CENTER)
                , fontID(Font::POPPINS_REGULAR_18)
            {}

            Point  p;
            Size   s;
            AlighH alignH;
            AlighV alignV;
            Font   fontID;
        };

        //// API
        virtual Size getRegionSize()                                                       = 0;
        virtual void fill(const Region& r, uint8_t color)                                  = 0;
        virtual void line(const Point& from, const Point& to, uint8_t color)               = 0;
        virtual void circle(const Point& center, uint16_t radius, uint8_t color)           = 0;
        virtual void text(const Region&  r, const char* str)                               = 0;
        virtual void image(const Region& r, uint16_t w, uint16_t h, const uint8_t* image)  = 0;
        virtual void image(const Region& r, uint16_t w, uint16_t h, const char* file)      = 0;
    };

}

#endif
