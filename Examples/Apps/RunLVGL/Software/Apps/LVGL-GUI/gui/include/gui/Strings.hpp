/**
 ******************************************************************************
 * @file    Strings.hpp
 * @brief   User-visible strings shared by more than one screen.
 *
 * The TouchGFX Run app keeps these in texts.xml; here they are plain UTF-8
 * literals, which LVGL renders directly (every generated font covers ASCII).
 ******************************************************************************
 */

#ifndef STRINGS_HPP
#define STRINGS_HPP

namespace Strings
{
inline constexpr const char* kAppNameUc = "RUNLVGL";
inline constexpr const char* kKm        = "km";
inline constexpr const char* kMi        = "mi";
inline constexpr const char* kNoValue   = "---";
} // namespace Strings

#endif // STRINGS_HPP
