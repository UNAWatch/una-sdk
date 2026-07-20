#include <gui/containers/OptionWheelCenterItem.hpp>

OptionWheelCenterItem::OptionWheelCenterItem()
{
    mTextBuffer[0] = 0;
}

void OptionWheelCenterItem::initialize()
{
    OptionWheelCenterItemBase::initialize();
}

void OptionWheelCenterItem::apply(const OptionWheelConfig& cfg)
{
    switch (cfg.style) {
    case OptionWheelConfig::SIMPLE:
        text.setWildcard(mTextBuffer);
        if (cfg.rawText != nullptr) {
            Unicode::snprintf(mTextBuffer, kTextSize, "%s", cfg.rawText);
        } else if (cfg.msgId != TYPED_TEXT_INVALID) {
            Unicode::snprintf(mTextBuffer, kTextSize, "%s", touchgfx::TypedText(cfg.msgId).getText());
        }
        // Shrink to the largest size that fits the fixed-width item so long
        // labels (e.g. "Beep & Vibrate") are not clipped.
        text.setTypedText(touchgfx::TypedText(T_TMP_SEMIBOLD_30));
        if (text.getTextWidth() > text.getWidth()) {
            text.setTypedText(touchgfx::TypedText(T_TMP_SEMIBOLD_25));
        }
        if (text.getTextWidth() > text.getWidth()) {
            text.setTypedText(touchgfx::TypedText(T_TMP_SEMIBOLD_20));
        }
        text.setVisible(true);
        toggle.setVisible(false);
        textToggleLeft.setVisible(false);
        textToggleRight.setVisible(false);
        break;

    case OptionWheelConfig::TOGGLE:
        if (cfg.msgIdLeft != TYPED_TEXT_INVALID) {
            Unicode::snprintf(textToggleLeftBuffer, TEXTTOGGLELEFT_SIZE, "%s",
                              touchgfx::TypedText(cfg.msgIdLeft).getText());
        }
        if (cfg.msgIdRight != TYPED_TEXT_INVALID) {
            Unicode::snprintf(textToggleRightBuffer, TEXTTOGGLERIGHT_SIZE, "%s",
                              touchgfx::TypedText(cfg.msgIdRight).getText());
        }
        toggle.setState(cfg.toggleState);
        text.setVisible(false);
        toggle.setVisible(true);
        textToggleLeft.setVisible(true);
        textToggleRight.setVisible(true);
        break;
    }

    text.invalidate();
    toggle.invalidate();
    textToggleLeft.invalidate();
    textToggleRight.invalidate();
}
