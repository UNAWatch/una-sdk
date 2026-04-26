/**
 * @file Config.hpp
 * @date 22-04-2026
 * @author Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief SDK-level GUI configuration constants.
 *
 * Contains constants that describe the kernel/SDK environment for GUI
 * applications, such as the display tick rate. Applications should
 * reference these values rather than hardcoding them.
 */

#ifndef SDK_GUI_CONFIG_HPP
#define SDK_GUI_CONFIG_HPP

#include <cstdint>

namespace SDK::GUI::Config
{

/**
 * @brief Kernel tick rate delivered to GUI applications (frames per second).
 *
 * The kernel calls the application's tick handler this many times per second.
 * Use this value when computing durations in ticks.
 */
inline constexpr uint32_t kFrameRate = 10;

} // namespace SDK::GUI::Config

#endif // SDK_GUI_CONFIG_HPP
