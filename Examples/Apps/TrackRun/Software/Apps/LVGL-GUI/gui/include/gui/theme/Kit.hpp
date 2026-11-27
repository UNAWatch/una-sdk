/**
 ******************************************************************************
 * @file    Kit.hpp
 * @brief   Track Run's design kit in code: the design's colours, type ramp
 *          and track ring.
 *
 * Colours: the kit's variables are the watch's 64 colours written in 0x55
 * steps (#AA5555); the SDK writes the same 2-bit values in 0x40 steps
 * (SDK/GUI/Color.hpp), so each maps exactly. The panel (Sharp LS012B7DD06A,
 * reflective, no backlight) shows the darker mid-tones (Selection, Muted)
 * dimmer than the design does: important text stays on black, in Text, Neutral,
 * Accent or Teal.
 *
 * Type: the kit's Poppins styles on the faces compiled into the app. Choice
 * 20 uses SemiBold 20 (no Medium 20), and the lane badge's 13 uses Regular
 * 14. The run screen's header is Regular 14 (the panel's design rules have
 * no italic for small text), so there is no Italic 14 face.
 ******************************************************************************
 */

#ifndef TRACKRUN_KIT_HPP
#define TRACKRUN_KIT_HPP

#include <cstdint>

#include "SDK/GUI/Color.hpp"
#include "gui/theme/Theme.hpp"

namespace Kit
{

// Colour variables (the design's value in the comment).
constexpr uint32_t kBackground = SDK::GUI::Color::BLACK;       ///< #000000 screen background
constexpr uint32_t kText       = SDK::GUI::Color::WHITE;       ///< #FFFFFF primary text, lane lines
constexpr uint32_t kNeutral    = SDK::GUI::Color::GRAY;        ///< #AAAAAA secondary text, labels
constexpr uint32_t kMuted      = SDK::GUI::Color::GRAY_DARK;   ///< #555555 road ring, empty bars, the even chip
constexpr uint32_t kTeal       = SDK::GUI::Color::TEAL;        ///< #00AAAA dividers, progress, headers
constexpr uint32_t kSelection  = SDK::GUI::Color::TEAL_DARK;   ///< #005555 selected row band
constexpr uint32_t kAccent     = SDK::GUI::Color::YELLOW_DARK; ///< #FFAA00 calibrating, rest, warnings
constexpr uint32_t kTartan     = SDK::GUI::Color::SIENNA;      ///< #AA5555 the track ring, lane badges
constexpr uint32_t kFaster     = SDK::GUI::Color::LIME_SOFT;   ///< #55FF55 faster than the last
constexpr uint32_t kSlower     = SDK::GUI::Color::SALMON;      ///< #FF5555 slower than the last
constexpr uint32_t kOnAccent   = SDK::GUI::Color::BLACK;       ///< #000000 text on coloured fills
// A light fill takes black text (the panel rule). The palette's
// lightest coral, as bright on the panel as the green: the lane badge and
// the slower chip (the ring keeps tartan; the flash and the summary keep
// Slower).
constexpr uint32_t kLaneFill   = SDK::GUI::Color::ROSE;        ///< #FFAAAA lane badges, the slower chip

// The ring's lane line is white at 55% in the design; the blend is not one of the
// 64 colours, so the nearest is used: over tartan ROSE, over teal CYAN.
constexpr uint32_t kLaneOnTartan = SDK::GUI::Color::ROSE;      ///< C08080
constexpr uint32_t kLaneOnTeal   = SDK::GUI::Color::CYAN;      ///< 40C0C0

// The type ramp.
using F = Theme::Font;
constexpr F kHeader = F::Regular14;   ///< Header 14 (italic in the design; Regular on the watch)
constexpr F kSmall  = F::Regular14;   ///< Small 14
constexpr F kCaption = F::Regular16;  ///< Caption 16
constexpr F kBody   = F::Medium18;    ///< Body 18
constexpr F kChoice = F::SemiBold20;  ///< Choice 20 (Medium 20 not compiled)
constexpr F kAction = F::SemiBold25;  ///< Action 25
constexpr F kTitle  = F::SemiBold30;  ///< Title 30
constexpr F kBig    = F::SemiBold40;  ///< Big 40 (digits only)
constexpr F kHuge   = F::SemiBold60;  ///< Huge 60 (digits only)

// The track ring (240 x 240): a tartan band from radius
// 106 to 114 with a lane line on its middle, a start line at 12 o'clock,
// the runner's dot on the band, Teal progress over the band. Road: a 3 px
// Muted ring at the outer edge. Calibrating: four quarters with gaps.
constexpr int32_t kRingCx         = 120;
constexpr int32_t kRingCy         = 120;
constexpr int32_t kRingRadius     = 110;   ///< The band's centre line, and the dot's
constexpr int32_t kRingWidth      = 8;
constexpr int32_t kLaneWidth      = 2;     ///< 0.8 px in the design; see TrackRing
constexpr int32_t kRoadRadius     = 112;   ///< 111 to 114
constexpr int32_t kRoadWidth      = 3;
constexpr int32_t kDotSize        = 14;    ///< 10 px white, 2 px black edge
constexpr int32_t kStartLineW     = 3;
constexpr int32_t kStartLineH     = 12;
constexpr int32_t kStartLineY     = 4;
constexpr int32_t kCalGapDeg      = 3;     ///< Either side of each quarter's ends

// The divider: Teal, 150 x 2.
constexpr int32_t kDividerX = 45;
constexpr int32_t kDividerW = 150;
constexpr int32_t kDividerH = 2;

} // namespace Kit

#endif // TRACKRUN_KIT_HPP
