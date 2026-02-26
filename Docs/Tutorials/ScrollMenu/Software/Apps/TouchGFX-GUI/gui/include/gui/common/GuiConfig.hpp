/**
 ******************************************************************************
 * @file    Config.hpp
 * @date    19-01-2024
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Configuration for GUI.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __GUI_CONFIG_HPP
#define __GUI_CONFIG_HPP

#include <cstdint>
#include <cstdbool>

#include "touchgfx/Color.hpp"
#include <texts/TextKeysAndLanguages.hpp>

#define GUI_CONFIG_MS_2_TICKS(ms) ((ms)/(1000 / Gui::Config::kFrameRate))
namespace Gui
{

namespace Config
{

constexpr uint32_t kFrameRate = 10;
constexpr uint32_t kMenuAnimationSteps = 4;
constexpr uint32_t kScreenTimeoutSteps = GUI_CONFIG_MS_2_TICKS(30000);     // 30s


namespace Button
{
constexpr uint8_t L1 = '1';
constexpr uint8_t L2 = '2';
constexpr uint8_t R1 = '3';
constexpr uint8_t R2 = '4';
constexpr uint8_t L1R2 = 'z';
} // Button


namespace Color
{
inline const touchgfx::colortype WHITE = 0xC0C0C0;
inline const touchgfx::colortype GRAY = 0x808080;
inline const touchgfx::colortype GRAY_DARK = 0x404040;
inline const touchgfx::colortype BLACK = 0x000000;
inline const touchgfx::colortype YELLOW_DARK = 0xC08000;
inline const touchgfx::colortype TEAL = 0x008080;
inline const touchgfx::colortype TEAL_DARK = 0x004040;
inline const touchgfx::colortype GREEN = 0x00C000;
inline const touchgfx::colortype DARK_GREEN = 0x004000;
inline const touchgfx::colortype BROUN = 0xC08040;
inline const touchgfx::colortype RED = 0xC00000;
inline const touchgfx::colortype DARK_RED = 0x400000;
inline const touchgfx::colortype BLUE = 0x0000C0;
inline const touchgfx::colortype DARK_BLUE = 0x000040;
inline const touchgfx::colortype CYAN = 0x40C0C0;
inline const touchgfx::colortype CYAN_LIGHT = 0x80C0C0;
} // namespace Color

} // namespace Config

} // namespace Gui

#endif /* __GUI_CONFIG_HPP */
