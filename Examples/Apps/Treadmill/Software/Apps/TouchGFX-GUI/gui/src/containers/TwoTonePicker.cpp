#include <gui/containers/TwoTonePicker.hpp>
#include <gui/containers/Buttons.hpp>
#include <touchgfx/Color.hpp>
#include <touchgfx/Font.hpp>

namespace {

// Value / upcoming-value row geometry, mirroring TwoTonePickerBase so the extra
// slot renderValue3() adds lines up with the generated ones.
constexpr int16_t kValY   = 92;    ///< Big value row.
constexpr int16_t kValH   = 80;
constexpr int16_t kFracX  = 127;   ///< Left edge of the fraction, just past the separator.
constexpr int16_t kLeftX  = 5;     ///< Left (whole-part) column.
constexpr int16_t kLeftW  = 110;
constexpr int16_t kRightW = 108;   ///< Full-width right column (two-stage layout).
constexpr int16_t kUpY    = 151;   ///< First upcoming-value row.
constexpr int16_t kUpH    = 50;
constexpr int16_t kUp2Y   = 193;   ///< Second upcoming-value row.
constexpr int16_t kUp2H   = 36;

/// Poppins SemiBold 60 digit advance -- only used if the font is not bound yet.
constexpr uint16_t kFallbackDigitW = 40;

} // namespace

touchgfx::colortype TwoTonePicker::teal() { return touchgfx::Color::getColorFromRGB(0, 128, 128); }
touchgfx::colortype TwoTonePicker::grey() { return touchgfx::Color::getColorFromRGB(192, 192, 192); }

uint16_t TwoTonePicker::digitSlotWidth()
{
    // Poppins figures are tabular, so any digit gives the slot width.
    const touchgfx::Font* font = touchgfx::TypedText(T_TMP_SEMIBOLD_60).getFont();
    const uint16_t width = (font != nullptr) ? font->getCharWidth('0') : 0;
    return (width != 0) ? width : kFallbackDigitW;
}

TwoTonePicker::TwoTonePicker()
{
    // Hundredths slot: positioned per-render by renderValue3(). Added here rather
    // than in initialize() so it is registered exactly once per construction.
    // Screens that call renderValue() never write its buffer, so it stays blank.
    valFracLoBuffer[0] = 0;
    valFracLo.setPosition(kFracX + kFallbackDigitW, kValY, kFallbackDigitW, kValH);
    valFracLo.setColor(grey());
    valFracLo.setLinespacing(0);
    valFracLo.setWildcard(valFracLoBuffer);
    valFracLo.setTypedText(touchgfx::TypedText(T_TMP_LIGHT_60));
    add(valFracLo);
}

void TwoTonePicker::initialize()
{
    TwoTonePickerBase::initialize();

    // Fixed bezel mapping for every picker: left = scroll, R1 = confirm/advance
    // (amber), R2 = skip / step back.
    buttons.setL1(Buttons::WHITE);
    buttons.setL2(Buttons::WHITE);
    buttons.setR1(Buttons::AMBER);
    buttons.setR2(Buttons::WHITE);
}

void TwoTonePicker::setTitle(TypedTextId titleId)
{
    title.set(titleId);
}

void TwoTonePicker::renderSubtitleSingle(TypedTextId label)
{
    // One centred teal label spanning the full width.
    subLeft.setPosition(20, 58, 200, 28);
    subLeft.setTypedText(touchgfx::TypedText(T_TMP_ITALIC_20));
    Unicode::snprintf(subLeftBuffer, SUBLEFT_SIZE, "%s",
        touchgfx::TypedText(label).getText());
    subLeft.setWildcard(subLeftBuffer);
    subLeft.setColor(teal());
    subLeft.invalidate();

    subRightBuffer[0] = 0;
    subRight.setWildcard(subRightBuffer);
    subRight.invalidate();
}

void TwoTonePicker::renderSubtitleDual(TypedTextId left, TypedTextId right, bool leftActive)
{
    // Two labels, one centred over each value column; active one teal.
    subLeft.setPosition(5, 58, 110, 28);
    subLeft.setTypedText(touchgfx::TypedText(T_TMP_ITALIC_20));
    Unicode::snprintf(subLeftBuffer, SUBLEFT_SIZE, "%s",
        touchgfx::TypedText(left).getText());
    subLeft.setWildcard(subLeftBuffer);
    subLeft.setColor(leftActive ? teal() : grey());
    subLeft.invalidate();

    subRight.setPosition(127, 58, 108, 28);
    subRight.setTypedText(touchgfx::TypedText(T_TMP_ITALIC_20));
    Unicode::snprintf(subRightBuffer, SUBRIGHT_SIZE, "%s",
        touchgfx::TypedText(right).getText());
    subRight.setWildcard(subRightBuffer);
    subRight.setColor(leftActive ? grey() : teal());
    subRight.invalidate();
}

void TwoTonePicker::renderValue(bool leftActive,
                                const char* left, const char* right, const char* sep,
                                const char* up1, const char* up2)
{
    // Separator: always grey, light. (Unicode::strncpy converts ASCII -> UnicodeChar;
    // Unicode::snprintf "%s" expects a UnicodeChar*, so it must not be used here.)
    Unicode::strncpy(valSepBuffer, sep, VALSEP_SIZE);
    valSep.setColor(grey());

    // Left component: teal SemiBold when active, grey Light otherwise
    // (right-aligned to the separator).
    Unicode::strncpy(valLeftBuffer, left, VALLEFT_SIZE);
    valLeft.setTypedText(touchgfx::TypedText(leftActive ? T_TMP_SEMIBOLD_60_R : T_TMP_LIGHT_60_R));
    valLeft.setWildcard(valLeftBuffer);
    valLeft.setColor(leftActive ? teal() : grey());

    // Undo the three-slot layout in case this picker previously rendered it: give
    // valRight its full-width box back and blank the hundredths slot, which would
    // otherwise stay painted on top of it.
    valFracLo.invalidate();
    valFracLoBuffer[0] = 0;
    valFracLo.setWildcard(valFracLoBuffer);

    // Right component: teal SemiBold when active, grey Light otherwise
    // (left-aligned from the separator).
    valRight.invalidate();
    valRight.setPosition(kFracX, kValY, kRightW, kValH);
    Unicode::strncpy(valRightBuffer, right, VALRIGHT_SIZE);
    valRight.setTypedText(touchgfx::TypedText(leftActive ? T_TMP_LIGHT_60_L : T_TMP_SEMIBOLD_60_L));
    valRight.setWildcard(valRightBuffer);
    valRight.setColor(leftActive ? grey() : teal());

    // Clear the OLD upcoming-value rects BEFORE repositioning, else TouchGFX
    // leaves the vacated area unpainted (leftover-digits artifact).
    nextVal.invalidate();
    nextVal2.invalidate();

    if (leftActive) {
        // Right-aligned under the left component (pulls toward centre).
        nextVal.setTypedText(touchgfx::TypedText(T_TMP_MEDIUM_40_R));
        nextVal2.setTypedText(touchgfx::TypedText(T_TMP_MEDIUM_25_R));
        nextVal.setPosition(5, 151, 110, 50);
        nextVal2.setPosition(5, 193, 110, 36);
    } else {
        // Left-aligned under the right component.
        nextVal.setTypedText(touchgfx::TypedText(T_TMP_MEDIUM_40_L));
        nextVal2.setTypedText(touchgfx::TypedText(T_TMP_MEDIUM_25_L));
        nextVal.setPosition(127, 151, 108, 50);
        nextVal2.setPosition(127, 193, 108, 36);
    }

    Unicode::strncpy(nextValBuffer, up1 ? up1 : "", NEXTVAL_SIZE);
    Unicode::strncpy(nextVal2Buffer, up2 ? up2 : "", NEXTVAL2_SIZE);
    nextVal.setWildcard(nextValBuffer);
    nextVal2.setWildcard(nextVal2Buffer);
    nextVal.setColor(grey());
    nextVal2.setColor(grey());

    valLeft.invalidate();
    valSep.invalidate();
    valRight.invalidate();
    nextVal.invalidate();
    nextVal2.invalidate();
}

void TwoTonePicker::renderValue3(Segment active,
                                 const char* whole, const char* fracHi, const char* fracLo,
                                 const char* up1, const char* up2)
{
    const int16_t slotW = static_cast<int16_t>(digitSlotWidth());
    const int16_t hiX   = kFracX;
    const int16_t loX   = static_cast<int16_t>(kFracX + slotW);

    const bool wholeActive = (active == SEG_WHOLE);
    const bool hiActive    = (active == SEG_FRAC_HI);
    const bool loActive    = (active == SEG_FRAC_LO);

    // Separator: always grey, light. (Unicode::strncpy converts ASCII -> UnicodeChar;
    // Unicode::snprintf "%s" expects a UnicodeChar*, so it must not be used here.)
    Unicode::strncpy(valSepBuffer, ".", VALSEP_SIZE);
    valSep.setColor(grey());

    // Whole part: right-aligned into the separator, as in the two-stage layout.
    Unicode::strncpy(valLeftBuffer, whole, VALLEFT_SIZE);
    valLeft.setTypedText(touchgfx::TypedText(wholeActive ? T_TMP_SEMIBOLD_60_R : T_TMP_LIGHT_60_R));
    valLeft.setWildcard(valLeftBuffer);
    valLeft.setColor(wholeActive ? teal() : grey());

    // Fraction digits: one digit centred in each fixed-width slot. Sizing the slot
    // to the SemiBold advance (Light digits are a few px narrower) keeps both digits
    // put as the active weight moves between them. Clear the old rects BEFORE
    // resizing valRight down from its full-width box, else TouchGFX leaves the
    // vacated area unpainted (leftover-digits artifact).
    valRight.invalidate();
    valRight.setPosition(hiX, kValY, slotW, kValH);
    valRight.setTypedText(touchgfx::TypedText(hiActive ? T_TMP_SEMIBOLD_60 : T_TMP_LIGHT_60));
    Unicode::strncpy(valRightBuffer, fracHi, VALRIGHT_SIZE);
    valRight.setWildcard(valRightBuffer);
    valRight.setColor(hiActive ? teal() : grey());

    valFracLo.invalidate();
    valFracLo.setPosition(loX, kValY, slotW, kValH);
    valFracLo.setTypedText(touchgfx::TypedText(loActive ? T_TMP_SEMIBOLD_60 : T_TMP_LIGHT_60));
    Unicode::strncpy(valFracLoBuffer, fracLo, VALFRACLO_SIZE);
    valFracLo.setWildcard(valFracLoBuffer);
    valFracLo.setColor(loActive ? teal() : grey());

    nextVal.invalidate();
    nextVal2.invalidate();

    if (wholeActive) {
        // Right-aligned under the whole part (pulls toward centre).
        nextVal.setTypedText(touchgfx::TypedText(T_TMP_MEDIUM_40_R));
        nextVal2.setTypedText(touchgfx::TypedText(T_TMP_MEDIUM_25_R));
        nextVal.setPosition(kLeftX, kUpY, kLeftW, kUpH);
        nextVal2.setPosition(kLeftX, kUp2Y, kLeftW, kUp2H);
    } else {
        // Centred under whichever fraction digit is being edited.
        const int16_t x = hiActive ? hiX : loX;
        nextVal.setTypedText(touchgfx::TypedText(T_TMP_MEDIUM_40));
        nextVal2.setTypedText(touchgfx::TypedText(T_TMP_MEDIUM_25));
        nextVal.setPosition(x, kUpY, slotW, kUpH);
        nextVal2.setPosition(x, kUp2Y, slotW, kUp2H);
    }

    Unicode::strncpy(nextValBuffer, up1 ? up1 : "", NEXTVAL_SIZE);
    Unicode::strncpy(nextVal2Buffer, up2 ? up2 : "", NEXTVAL2_SIZE);
    nextVal.setWildcard(nextValBuffer);
    nextVal2.setWildcard(nextVal2Buffer);
    nextVal.setColor(grey());
    nextVal2.setColor(grey());

    valLeft.invalidate();
    valSep.invalidate();
    valRight.invalidate();
    valFracLo.invalidate();
    nextVal.invalidate();
    nextVal2.invalidate();
}
