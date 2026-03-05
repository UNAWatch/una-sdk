#include <gui/containers/MenuItemNotSelected.hpp>
#include <texts/TextKeysAndLanguages.hpp>

MenuItemNotSelected::MenuItemNotSelected()
{

}

void MenuItemNotSelected::initialize()
{
    MenuItemNotSelectedBase::initialize();

    reset();
    mMsgIdType = text.getTypedText().getId();
    mMsgIdTipType = textTip.getTypedText().getId();
    mTipColor = textTip.getColor();
    mIcon.setVisible(false);
    add(mIcon);
}

void MenuItemNotSelected::config(TypedTextId msgId)
{
    reset();

    mMsgId = msgId;
    mStyle = STYLE_SIMPLE;
}

void MenuItemNotSelected::config(touchgfx::Unicode::UnicodeChar *msg)
{
    reset();

    if (msg != nullptr) {
        Unicode::snprintf(textBuffer, TEXT_SIZE, "%s", msg);
    }
    mStyle = STYLE_SIMPLE;
}

void MenuItemNotSelected::configTip(TypedTextId msgId, TypedTextId msgIdTip)
{
    reset();

    mMsgId = msgId;
    mMsgIdTip = msgIdTip;
    mStyle = STYLE_TIP;
}

void MenuItemNotSelected::configTip(TypedTextId msgId, touchgfx::Unicode::UnicodeChar *msgTip)
{
    reset();

    mMsgId = msgId;
    if (msgTip != nullptr) {
        Unicode::snprintf(textTipBuffer, TEXTTIP_SIZE, "%s", msgTip);
    }
    mStyle = STYLE_TIP;
}

void MenuItemNotSelected::setMsgTypedTextId(TypedTextId msgIdType)
{
    if (msgIdType != TYPED_TEXT_INVALID) {
        mMsgIdType = msgIdType;
    } else { 
        mMsgIdType = T_TMP_MEDIUM_18;
    }
}

void MenuItemNotSelected::setTipTypedTextId(TypedTextId msgIdType)
{
    if (msgIdType != TYPED_TEXT_INVALID) {
        mMsgIdTipType = msgIdType;
    } else {
        mMsgIdTipType = T_TMP_ITALIC_18;
    }
}

void MenuItemNotSelected::setTipColor(touchgfx::colortype color)
{
    mTipColor = color;
}

void MenuItemNotSelected::setIcon(touchgfx::BitmapId id)
{
    mIcon.setBitmap(touchgfx::Bitmap(id));
    mIcon.setVisible(true);
    mIcon.setPosition(5, (getHeight() - 32) / 2, 32, 32);
    invalidate();
}

void MenuItemNotSelected::reset()
{
    mStyle = STYLE_SIMPLE;

    mMsgId = TYPED_TEXT_INVALID;
    mMsgIdType = T_TMP_MEDIUM_18;
    textBuffer[0] = 0;

    mMsgIdTip = TYPED_TEXT_INVALID;
    mMsgIdTipType = T_TMP_ITALIC_18;
    textTipBuffer[0] = 0;

    mIcon.setVisible(false);
}

void MenuItemNotSelected::updStyle()
{
    if (mMsgId != TYPED_TEXT_INVALID) {
        Unicode::snprintf(textBuffer, TEXT_SIZE, "%s", touchgfx::TypedText(mMsgId).getText());
    }

    if (mMsgIdTip != TYPED_TEXT_INVALID) {
        Unicode::snprintf(textTipBuffer, TEXTTIP_SIZE, "%s", touchgfx::TypedText(mMsgIdTip).getText());
    }

    if (mMsgIdType != TYPED_TEXT_INVALID) {
        text.setTypedText(touchgfx::TypedText(mMsgIdType));
    }

    if (mMsgIdTipType != TYPED_TEXT_INVALID) {
        textTip.setTypedText(touchgfx::TypedText(mMsgIdTipType));
    }

    textTip.setColor(mTipColor);

    switch (mStyle) {
        case STYLE_SIMPLE:
            text.setVisible(true);
            text.setWidth(getWidth() - 40);
            text.setX(40);
            textTip.setVisible(false);
            break;
        case STYLE_TIP:
            text.setVisible(true);
            text.setWidth(getWidth() - 40);
            text.setX(40);
            textTip.setVisible(true);
            break;
        default:
            break;
    };

    text.invalidate();
    textTip.invalidate();
}

MenuItemNotSelected &MenuItemNotSelected::operator=(const MenuItemNotSelected &other)
{
    if (this != &other) {
        mStyle = other.mStyle;

        mMsgId = other.mMsgId;
        mMsgIdType = other.mMsgIdType;
        Unicode::strncpy(textBuffer, other.textBuffer, TEXT_SIZE);

        mMsgIdTip = other.mMsgIdTip;
        mMsgIdTipType = other.mMsgIdTipType;
        Unicode::strncpy(textTipBuffer, other.textTipBuffer, TEXTTIP_SIZE);
        mTipColor = other.mTipColor;

        mIcon.setBitmap(other.mIcon.getBitmap());
        mIcon.setPosition(other.mIcon.getX(), other.mIcon.getY(), other.mIcon.getWidth(), other.mIcon.getHeight());
        mIcon.setVisible(other.mIcon.isVisible());

        updStyle();
    }
    return *this;
}
