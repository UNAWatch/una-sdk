#include <gui/containers/OptionWheelItem.hpp>

OptionWheelItem::OptionWheelItem()
{
}

void OptionWheelItem::initialize()
{
    OptionWheelItemBase::initialize();
}

void OptionWheelItem::apply(const OptionWheelConfig& cfg)
{
    if (cfg.msgId != TYPED_TEXT_INVALID) {
        Unicode::snprintf(textBuffer, TEXT_SIZE, "%s", touchgfx::TypedText(cfg.msgId).getText());
    }
    text.invalidate();
}
