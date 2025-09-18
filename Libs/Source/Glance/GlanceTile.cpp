/**
 ******************************************************************************
 * @file    GlanceTile.cpp
 * @date    18-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Class represents only one tile on the Glance area
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */


#include "SDK/Glance/GlanceTile.hpp"

#include <cstdarg>
#include <cstdio>

namespace SDK::Glance {

Tile::Tile(SDK::Interface::IGlance& glance, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
    : mGlance(glance)
    , mRegion(x, y, w, h)
{
}

Tile& Tile::reinit(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    mRegion.p.x = x;
    mRegion.p.y = y;

    mRegion.s.w = w;
    mRegion.s.h = h;

    return *this;
}

Tile& Tile::fill(uint16_t color)
{
    mGlance.fill(mRegion, color);

    return *this;
}

Tile& Tile::setFont(SDK::Interface::IGlance::Font id)
{
    mRegion.fontID = id;

    return *this;
}

Tile& Tile::setAlign(SDK::Interface::IGlance::AlighH h)
{
    mRegion.alignH = h;

    return *this;
}

Tile& Tile::setAlign(SDK::Interface::IGlance::AlighV v)
{
    mRegion.alignV = v;

    return *this;
}

void Tile::setImage(uint16_t w, uint16_t h, const uint8_t* image)
{
    mGlance.image(mRegion, w, h, image);
}

void Tile::setImage(uint16_t w, uint16_t h, const char* file)
{
    mGlance.image(mRegion, w, h, file);
}

void Tile::setText(const char* str)
{
    mGlance.text(mRegion, str);
}

void Tile::print(const char* format, ...)
{
    if (!format) {
        return;
    }

    char buf[64];           // includes space for the null terminator
    buf[0] = '\0';

    va_list args;
    va_start(args, format);

#if defined(_MSC_VER)
    // MSVC-safe variant that truncates automatically
    const int written = _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, format, args);
#else
    // C99/POSIX: returns number of chars that would have been written (excluding '\0')
    const int written = std::vsnprintf(buf, sizeof(buf), format, args);
#endif
    va_end(args);

    // Ensure null-termination even if encoding failed or result was truncated
    if (written < 0 || written >= static_cast<int>(sizeof(buf))) {
        return;
    }

    setText(buf);
}

}
