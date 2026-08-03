#ifndef TWOTONEPICKER_HPP
#define TWOTONEPICKER_HPP

#include <gui_generated/containers/TwoTonePickerBase.hpp>

/**
 * @brief Shared two-stage "two-tone" value picker (Treadmill pickers).
 *
 * Renders a composite "<left><sep><right>" value where the *active* component is
 * teal SemiBold-60 and the inactive component + separator are grey Light-60, with
 * the next two upcoming values of the active component shown below it. The
 * upcoming column is right-aligned under the left component and left-aligned under
 * the right component so the digits pull toward the centre and clear the round
 * bezel.
 *
 * Two subtitle styles are supported:
 *   - single (distance): one centred teal label (e.g. "Kilometers" / "Miles")
 *   - dual   (time):     two labels (e.g. "Mins." / "Secs."), the active one teal,
 *                        the inactive one grey, one over each value column.
 *
 * Buttons are fixed: L1/L2 white (scroll), R1 amber (confirm/advance), R2 white
 * (skip / step back).
 *
 * The owning View holds the picker's value/stage state and calls renderValue() +
 * one of the renderSubtitle*() helpers on every change.
 */
class TwoTonePicker : public TwoTonePickerBase
{
public:
    /// Which slot of a three-part value is being edited (see renderValue3()).
    enum Segment : uint8_t { SEG_WHOLE = 0, SEG_FRAC_HI, SEG_FRAC_LO };

    TwoTonePicker();
    virtual ~TwoTonePicker() {}

    virtual void initialize();

    /// Set the screen title (e.g. T_TEXT_DISTANCE_UC / T_TEXT_TIME_UC).
    void setTitle(TypedTextId titleId);

    /// Single centred teal subtitle (distance pickers).
    void renderSubtitleSingle(TypedTextId label);

    /// Two subtitles over each column; the active one is teal, the other grey
    /// (time picker). @p leftActive selects which one is teal.
    void renderSubtitleDual(TypedTextId left, TypedTextId right, bool leftActive);

    /**
     * Render the big composite value and the two upcoming values.
     * @param leftActive  left component is the one being edited (teal SemiBold).
     * @param left/right  formatted strings for the two components.
     * @param sep         separator string ("." or ":").
     * @param up1/up2     upcoming values of the active component ("" to blank).
     */
    void renderValue(bool leftActive,
                     const char* left, const char* right, const char* sep,
                     const char* up1, const char* up2);

    /**
     * Render a three-slot "<whole>.<fracHi><fracLo>" value, where any one of the
     * three slots is the active (teal SemiBold) one -- used by the Calibrate & Save
     * picker, which edits the whole part, tenths and hundredths separately.
     *
     * The two fraction digits get one fixed-width centred slot each, sized to the
     * SemiBold digit advance, so they hold their position as the active weight moves
     * between them; the upcoming column is centred under the active digit instead of
     * left-aligned across both.
     *
     * @param active           slot being edited.
     * @param whole            formatted whole part (right-aligned into the separator).
     * @param fracHi/fracLo    single tenths / hundredths digit.
     * @param up1/up2          upcoming values of the active slot ("" to blank).
     */
    void renderValue3(Segment active,
                      const char* whole, const char* fracHi, const char* fracLo,
                      const char* up1, const char* up2);

private:
    static touchgfx::colortype teal();
    static touchgfx::colortype grey();

    /// Advance width of one 60px digit, i.e. the width of one fraction slot.
    static uint16_t digitSlotWidth();

    /// Fourth value slot: the hundredths digit. The generated base only provides
    /// left / separator / right, which is all the two-stage pickers need.
    touchgfx::TextAreaWithOneWildcard valFracLo;
    static const uint16_t VALFRACLO_SIZE = 3;
    touchgfx::Unicode::UnicodeChar valFracLoBuffer[VALFRACLO_SIZE];
};

#endif // TWOTONEPICKER_HPP
