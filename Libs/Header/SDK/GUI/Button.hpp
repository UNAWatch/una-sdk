/**
 * @file Button.hpp
 * @date 22-04-2026
 * @author Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief Hardware button codes for SDK GUI applications.
 *
 * Single source of truth for button codes passed from the kernel to
 * applications via handleKeyEvent(). Use these constants instead of
 * raw character literals.
 *
 * Every code is a printable ASCII character so the Visual Studio simulator can
 * inject it straight from the PC keyboard. The codes form three keyboard rows,
 * one column per button, so a button keeps its column across events:
 *
 *      button   click   press   release
 *      L1        '1'     'q'      'a'
 *      L2        '2'     'w'      's'
 *      R1        '3'     'e'      'd'
 *      R2        '4'     'r'      'f'
 *
 * L1R2 ('z') is the simultaneous-click chord of L1 and R2. Long press is not a
 * code of its own: a screen derives it from the press/release pair with its
 * own timing.
 */

#ifndef SDK_GUI_BUTTON_HPP
#define SDK_GUI_BUTTON_HPP

#include <cstdint>

namespace SDK::GUI::Button
{

// Click codes: top digit row.
inline constexpr uint8_t L1   = '1';
inline constexpr uint8_t L2   = '2';
inline constexpr uint8_t R1   = '3';
inline constexpr uint8_t R2   = '4';
inline constexpr uint8_t L1R2 = 'z';

// Press codes: "qwer" row, directly under the digit row.
inline constexpr uint8_t L1_PRESS = 'q';
inline constexpr uint8_t L2_PRESS = 'w';
inline constexpr uint8_t R1_PRESS = 'e';
inline constexpr uint8_t R2_PRESS = 'r';

// Release codes: "asdf" home row, directly under the press row.
inline constexpr uint8_t L1_RELEASE = 'a';
inline constexpr uint8_t L2_RELEASE = 's';
inline constexpr uint8_t R1_RELEASE = 'd';
inline constexpr uint8_t R2_RELEASE = 'f';

/**
 * @brief True if the code is one of the button codes above.
 *
 * The simulator uses this to pass only real button keys from the keyboard,
 * so stray typing does not reach app logic.
 */
inline constexpr bool isButtonCode(uint8_t key)
{
    switch (key) {
        case L1: case L2: case R1: case R2: case L1R2:
        case L1_PRESS: case L2_PRESS: case R1_PRESS: case R2_PRESS:
        case L1_RELEASE: case L2_RELEASE: case R1_RELEASE: case R2_RELEASE:
            return true;
        default:
            return false;
    }
}

} // namespace SDK::GUI::Button

#endif // SDK_GUI_BUTTON_HPP
