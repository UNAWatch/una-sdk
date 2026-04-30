/**
 * @file Color.hpp
 * @date 22-04-2026
 * @author Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief Complete 64-color palette for SDK GUI applications.
 *
 * The display uses 6-bit color (2 bits per channel), giving four intensity
 * levels per channel: 0x00, 0x40, 0x80, 0xC0. The full palette is therefore
 * 4 x 4 x 4 = 64 colors, all enumerated here.
 *
 * Colors are organized by hue family. The hex comment on each line shows
 * the (R, G, B) byte triple for quick visual reference.
 *
 * NOTE: Declared as constexpr uint32_t (not inline const touchgfx::colortype)
 * because touchgfx::colortype has a non-constexpr constructor. Using
 * inline const colortype would generate a runtime initialisation guard (~35 B)
 * per color in every translation unit that includes this header, totalling
 * hundreds of kilobytes of wasted RAM. colortype(uint32_t) is implicit, so
 * all existing call-sites are unaffected by the type change.
 */

#ifndef SDK_GUI_COLOR_HPP
#define SDK_GUI_COLOR_HPP

#include <cstdint>

namespace SDK::GUI::Color
{


// =============================================================================
// Neutrals  (R = G = B)
// =============================================================================

constexpr uint32_t BLACK      = 0x000000;  // 00 00 00
constexpr uint32_t GRAY_DARK  = 0x404040;  // 40 40 40
constexpr uint32_t GRAY       = 0x808080;  // 80 80 80
constexpr uint32_t WHITE      = 0xC0C0C0;  // C0 C0 C0

// =============================================================================
// Reds  (G = 0, B = 0)
// =============================================================================

constexpr uint32_t RED_DARK   = 0x400000;  // 40 00 00
constexpr uint32_t MAROON     = 0x800000;  // 80 00 00
constexpr uint32_t RED        = 0xC00000;  // C0 00 00

// =============================================================================
// Greens  (R = 0, B = 0)
// =============================================================================

constexpr uint32_t GREEN_DARK = 0x004000;  // 00 40 00
constexpr uint32_t GREEN_MID  = 0x008000;  // 00 80 00
constexpr uint32_t GREEN      = 0x00C000;  // 00 C0 00

// =============================================================================
// Blues  (R = 0, G = 0)
// =============================================================================

constexpr uint32_t BLUE_DARK  = 0x000040;  // 00 00 40
constexpr uint32_t NAVY       = 0x000080;  // 00 00 80
constexpr uint32_t BLUE       = 0x0000C0;  // 00 00 C0

// =============================================================================
// Yellows  (B = 0, R > 0, G > 0)
// =============================================================================

constexpr uint32_t OLIVE_DARK  = 0x404000;  // 40 40 00
constexpr uint32_t MOSS        = 0x408000;  // 40 80 00
constexpr uint32_t CHARTREUSE  = 0x40C000;  // 40 C0 00

constexpr uint32_t SIENNA_DARK = 0x804000;  // 80 40 00
constexpr uint32_t OLIVE       = 0x808000;  // 80 80 00
constexpr uint32_t LIME        = 0x80C000;  // 80 C0 00

constexpr uint32_t ORANGE_DARK = 0xC04000;  // C0 40 00
constexpr uint32_t YELLOW_DARK = 0xC08000;  // C0 80 00
constexpr uint32_t YELLOW      = 0xC0C000;  // C0 C0 00

// =============================================================================
// Cyans / Teals  (R = 0, G > 0, B > 0)
// =============================================================================

constexpr uint32_t TEAL_DARK  = 0x004040;  // 00 40 40
constexpr uint32_t TEAL_BLUE  = 0x004080;  // 00 40 80
constexpr uint32_t AZURE      = 0x0040C0;  // 00 40 C0

constexpr uint32_t VIRIDIAN   = 0x008040;  // 00 80 40
constexpr uint32_t TEAL       = 0x008080;  // 00 80 80
constexpr uint32_t SKY        = 0x0080C0;  // 00 80 C0

constexpr uint32_t MINT       = 0x00C040;  // 00 C0 40
constexpr uint32_t AQUA       = 0x00C080;  // 00 C0 80
constexpr uint32_t CYAN_PURE  = 0x00C0C0;  // 00 C0 C0

// =============================================================================
// Magentas / Purples  (G = 0, R > 0, B > 0)
// =============================================================================

constexpr uint32_t PURPLE_DARK  = 0x400040;  // 40 00 40
constexpr uint32_t INDIGO       = 0x400080;  // 40 00 80
constexpr uint32_t VIOLET_DARK  = 0x4000C0;  // 40 00 C0

constexpr uint32_t PLUM         = 0x800040;  // 80 00 40
constexpr uint32_t PURPLE       = 0x800080;  // 80 00 80
constexpr uint32_t VIOLET       = 0x8000C0;  // 80 00 C0

constexpr uint32_t CRIMSON      = 0xC00040;  // C0 00 40
constexpr uint32_t PINK_DARK    = 0xC00080;  // C0 00 80
constexpr uint32_t MAGENTA      = 0xC000C0;  // C0 00 C0

// =============================================================================
// Mixed - low red  (R = 0x40, G > 0, B > 0)
// =============================================================================

constexpr uint32_t STEEL_DARK  = 0x404080;  // 40 40 80
constexpr uint32_t SLATE       = 0x4040C0;  // 40 40 C0

constexpr uint32_t SAGE_DARK   = 0x408040;  // 40 80 40
constexpr uint32_t STEEL       = 0x408080;  // 40 80 80
constexpr uint32_t CORNFLOWER  = 0x4080C0;  // 40 80 C0

constexpr uint32_t LIME_SOFT   = 0x40C040;  // 40 C0 40
constexpr uint32_t SEAFOAM     = 0x40C080;  // 40 C0 80
constexpr uint32_t CYAN        = 0x40C0C0;  // 40 C0 C0

// =============================================================================
// Mixed - mid red  (R = 0x80, G > 0, B > 0)
// =============================================================================

constexpr uint32_t SIENNA      = 0x804040;  // 80 40 40
constexpr uint32_t MAUVE       = 0x804080;  // 80 40 80
constexpr uint32_t LAVENDER    = 0x8040C0;  // 80 40 C0

constexpr uint32_t TAN         = 0x808040;  // 80 80 40
constexpr uint32_t PERIWINKLE  = 0x8080C0;  // 80 80 C0

constexpr uint32_t PISTACHIO   = 0x80C040;  // 80 C0 40
constexpr uint32_t SAGE        = 0x80C080;  // 80 C0 80
constexpr uint32_t CYAN_LIGHT  = 0x80C0C0;  // 80 C0 C0

// =============================================================================
// Mixed - full red  (R = 0xC0, G > 0, B > 0)
// =============================================================================

constexpr uint32_t SALMON      = 0xC04040;  // C0 40 40
constexpr uint32_t ROSE_DARK   = 0xC04080;  // C0 40 80
constexpr uint32_t ORCHID      = 0xC040C0;  // C0 40 C0

constexpr uint32_t BROWN       = 0xC08040;  // C0 80 40
constexpr uint32_t ROSE        = 0xC08080;  // C0 80 80
constexpr uint32_t PINK        = 0xC080C0;  // C0 80 C0

constexpr uint32_t LEMON       = 0xC0C040;  // C0 C0 40
constexpr uint32_t CREAM       = 0xC0C080;  // C0 C0 80

} // namespace SDK::GUI::Color

#endif // SDK_GUI_COLOR_HPP
