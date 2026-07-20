#include <gui/containers/OptionWheelItem.hpp>

OptionWheelItem::OptionWheelItem()
{
    mTextBuffer[0] = 0;
}

void OptionWheelItem::initialize()
{
    OptionWheelItemBase::initialize();
}

void OptionWheelItem::apply(const OptionWheelConfig& cfg)
{
    text.setWildcard(mTextBuffer);
    if (cfg.rawText != nullptr) {
        Unicode::snprintf(mTextBuffer, kTextSize, "%s", cfg.rawText);
    } else if (cfg.msgId != TYPED_TEXT_INVALID) {
        Unicode::snprintf(mTextBuffer, kTextSize, "%s", touchgfx::TypedText(cfg.msgId).getText());
    }
    text.invalidate();
}
