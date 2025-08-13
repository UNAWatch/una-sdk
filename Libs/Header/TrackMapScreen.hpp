/**
 ******************************************************************************
 * @file    TrackMapScreen.hpp
 * @date    12-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Represents a track in the form of screen pixels for ease of display.
 ******************************************************************************
 */

#ifndef __TRACK_MAP_SCREEN_HPP
#define __TRACK_MAP_SCREEN_HPP

#include <cstdint>
#include <vector>

 /**
  * @struct TrackMapScreen
  * @brief Represents a track in the form of screen pixels for ease of display.
  */
struct TrackMapScreen {

    // To save space, we use uint8_t to represent the point on screen.
    // However, for displays larger than 255x255, we need to use int16_t
#pragma pack(push, 1)
    /**
     * @struct ScreenPoint
     * @brief Represents a point on the screen with x and y coordinates.
     */
    struct Point {
        uint8_t x; ///< X-coordinate on the screen.
        uint8_t y; ///< Y-coordinate on the screen.
    };
#pragma pack(pop)

    std::vector<Point> points;  ///< Vector with track points.

    uint8_t minx; ///< Minimum X-coordinate on the screen.
    uint8_t miny; ///< Minimum Y-coordinate on the screen.
    uint8_t maxx; ///< Maximum X-coordinate on the screen.
    uint8_t maxy; ///< Maximum Y-coordinate on the screen.
};

#endif /* __TRACK_MAP_SCREEN_HPP */