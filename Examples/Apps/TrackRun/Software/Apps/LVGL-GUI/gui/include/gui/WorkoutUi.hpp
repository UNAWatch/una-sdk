/**
 ******************************************************************************
 * @file    WorkoutUi.hpp
 * @brief   Text and colours shared by the workout screens.
 *
 * The Run app's WorkoutUi, with UTF-8 strings for touchgfx::Unicode text and
 * LVGL fonts for TouchGFX typographies.
 ******************************************************************************
 */

#ifndef WORKOUT_UI_HPP
#define WORKOUT_UI_HPP

#include <cstddef>
#include <cstdint>

#include "lvgl.h"

#include "SDK/Workout/WorkoutLibrary.hpp"

namespace WorkoutUi
{

/// UTF-8 text from a workout file (a name or notes) as the fonts can draw
/// it: accented Latin letters lose their accents, typographic quotes and
/// dashes become plain ones, and any other character is '?'.
void toText(const char* utf8, char* out, size_t cap);

/// Width of @p text in @p font, in pixels.
int32_t textWidth(const char* text, const lv_font_t* font);

/// Cut @p buf (of @p cap bytes), ending it with an ellipsis, so it is at
/// most @p maxWidth pixels wide in @p font.
void fitWidth(char* buf, size_t cap, const lv_font_t* font, int32_t maxWidth);

/// Lay @p text over two lines, @p width1 and @p width2 pixels wide in
/// @p font, breaking between words; what does not fit is cut. Returns how
/// many bytes of @p text the first line used, spaces after it included, so
/// the rest of the text starts there.
size_t wrapTwo(const char* text, char* line1, char* line2, size_t cap, const lv_font_t* font,
               int32_t width1, int32_t width2);

/// The width a centred line from @p top to @p bottom can use on the round
/// display: the chord at its narrower end, less @p margin each side.
int32_t lineWidth(int32_t top, int32_t bottom, int32_t margin = 8);

/// The kind of step, for titles ("RUN", "REST", "WARM UP", "COOL DOWN").
const char* kindTitle(uint8_t intensity);

/// The kind of step, in a sentence ("Run", "Rest", "Warm Up", "Cool Down").
const char* kindLabel(uint8_t intensity);

/// Cyan for an active step, white for rest, warm-up and cool-down.
uint32_t kindColor(uint8_t intensity);

/// Green in range, red too fast or too high, amber too slow or too low,
/// grey without a reading or a target.
uint32_t zoneColor(uint8_t zone);

/// A planned date as "SAT 10 OCT".
void formatDate(uint32_t ymd, char* buf, size_t cap);

/// A workout's totals, e.g. "8.0 km, 45 min", with "~" in front when open
/// steps add an unknown amount; empty when it has none.
void formatTotals(const SDK::Workout::Library::Entry& entry, bool imperial, char* buf, size_t cap);

/// Split a step duration ("400 m") into its number ("400") and unit ("m").
/// One without a unit ("2:30") is all number.
void splitDuration(const char* duration, char* number, size_t numberCap, char* unit, size_t unitCap);

} // namespace WorkoutUi

#endif // WORKOUT_UI_HPP
