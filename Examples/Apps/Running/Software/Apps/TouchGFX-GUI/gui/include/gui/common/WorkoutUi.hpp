#ifndef WORKOUT_UI_HPP
#define WORKOUT_UI_HPP

#include <touchgfx/Unicode.hpp>
#include <touchgfx/hal/Types.hpp>
#include <texts/TextKeysAndLanguages.hpp>

#include "SDK/Workout/WorkoutLibrary.hpp"

#include <cstddef>
#include <cstdint>

/**
 * Text and colours shared by the workout screens.
 */
namespace WorkoutUi
{

/// UTF-8 text from a workout file (a name or notes) as the fonts can draw
/// it: accented Latin letters lose their accents, typographic quotes and
/// dashes become plain ones, and any other character is '?'.
void toText(const char* utf8, touchgfx::Unicode::UnicodeChar* out, uint16_t cap);

/// Cut @p buf, ending it with an ellipsis, so it is at most @p maxWidth
/// pixels wide in the font of @p typography.
void fitWidth(touchgfx::Unicode::UnicodeChar* buf, touchgfx::TypedTextId typography, int16_t maxWidth);

/// Lay @p text over two lines, @p width1 and @p width2 pixels wide in the font
/// of @p typography, breaking between words; what does not fit is cut.
void wrapTwo(const touchgfx::Unicode::UnicodeChar* text, touchgfx::Unicode::UnicodeChar* line1,
             touchgfx::Unicode::UnicodeChar* line2, uint16_t cap, touchgfx::TypedTextId typography,
             int16_t width1, int16_t width2);

/// The width a centred line from @p top to @p bottom can use on the round
/// display: the chord at its narrower end, less @p margin each side.
int16_t lineWidth(int16_t top, int16_t bottom, int16_t margin = 8);

/// The kind of step, for titles ("RUN", "REST", "WARM UP", "COOL DOWN").
touchgfx::TypedTextId kindTitle(uint8_t intensity);

/// The kind of step, in a sentence ("Run", "Rest", "Warm up", "Cool down").
touchgfx::TypedTextId kindLabel(uint8_t intensity);

/// Cyan for an active step, white for rest, warm-up and cool-down.
touchgfx::colortype kindColor(uint8_t intensity);

/// Green in range, red too fast or too high, amber too slow or too low,
/// grey without a reading or a target.
touchgfx::colortype zoneColor(uint8_t zone);

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
