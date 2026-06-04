#include <gui/containers/TwoTonePicker.hpp>
#include <gui/containers/Buttons.hpp>
#include <touchgfx/Color.hpp>

touchgfx::colortype TwoTonePicker::teal() { return touchgfx::Color::getColorFromRGB(0, 128, 128); }
touchgfx::colortype TwoTonePicker::grey() { return touchgfx::Color::getColorFromRGB(192, 192, 192); }

TwoTonePicker::TwoTonePicker() {}

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

    // Right component: teal SemiBold when active, grey Light otherwise
    // (left-aligned from the separator).
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
