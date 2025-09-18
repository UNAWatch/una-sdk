/**
 ******************************************************************************
 * @file    GlanceTile.hpp
 * @date    18-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Class represents only one tile on the Glance area
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __GLANCE_TILE_HPP
#define __GLANCE_TILE_HPP

#include "SDK/Interfaces/IGlance.hpp"

namespace SDK::Glance {

    class Tile {
    public:
        Tile(SDK::Interface::IGlance& glance, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

        Tile& reinit(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
        Tile& fill(uint16_t color);
        Tile& setFont(SDK::Interface::IGlance::Font id);
        Tile& setAlign(SDK::Interface::IGlance::AlighH h);
        Tile& setAlign(SDK::Interface::IGlance::AlighV v);
        void  setImage(uint16_t w, uint16_t h, const uint8_t* image);
        void  setImage(uint16_t w, uint16_t h, const char* file);
        void  setText(const char* str);
        void  print(const char* format, ...);

    private:
        SDK::Interface::IGlance&        mGlance;
        SDK::Interface::IGlance::Region mRegion;
    };

}

#endif
