#include <gui/containers/OptionWheelCenterItem.hpp>

OptionWheelCenterItem::OptionWheelCenterItem()
{
}

void OptionWheelCenterItem::initialize()
{
    OptionWheelCenterItemBase::initialize();
}

void OptionWheelCenterItem::apply(const OptionWheelConfig& cfg)
{
    switch (cfg.style) {
    case OptionWheelConfig::SIMPLE:
        if (cfg.msgId != TYPED_TEXT_INVALID) {
            Unicode::snprintf(textBuffer, TEXT_SIZE, "%s", touchgfx::TypedText(cfg.msgId).getText());
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
