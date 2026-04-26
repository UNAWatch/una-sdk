/**
 * @file Button.hpp
 * @date 22-04-2026
 * @author Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief Hardware button codes for SDK GUI applications.
 *
 * Single source of truth for button codes passed from the kernel to
 * applications via handleKeyEvent(). Use these constants instead of
 * raw character literals.
 */

#ifndef SDK_GUI_BUTTON_HPP
#define SDK_GUI_BUTTON_HPP

#include <cstdint>

namespace SDK::GUI::Button
{

inline constexpr uint8_t L1   = '1';
inline constexpr uint8_t L2   = '2';
inline constexpr uint8_t R1   = '3';
inline constexpr uint8_t R2   = '4';
inline constexpr uint8_t L1R2 = 'z';

} // namespace SDK::GUI::Button

#endif // SDK_GUI_BUTTON_HPP
