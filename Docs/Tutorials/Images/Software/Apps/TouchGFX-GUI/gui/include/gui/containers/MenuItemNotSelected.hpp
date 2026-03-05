#ifndef MENUITEMNOTSELECTED_HPP
#define MENUITEMNOTSELECTED_HPP

#include <gui_generated/containers/MenuItemNotSelectedBase.hpp>
#include <touchgfx/widgets/Image.hpp>
#include <touchgfx/Bitmap.hpp>
#include <touchgfx/Unicode.hpp>

class MenuItemNotSelected : public MenuItemNotSelectedBase
{
public:
    MenuItemNotSelected();
    virtual ~MenuItemNotSelected() {}

    virtual void initialize();

    void config(TypedTextId msgId);
    void config(touchgfx::Unicode::UnicodeChar *msg);

    void configTip(TypedTextId msgId, TypedTextId msgIdTip);
    void configTip(TypedTextId msgId, touchgfx::Unicode::UnicodeChar *msgTip);

    void setMsgTypedTextId(TypedTextId msgIdType);
    void setTipTypedTextId(TypedTextId msgIdType);
    void setTipColor(touchgfx::colortype color);

    void setIcon(touchgfx::BitmapId id);

    MenuItemNotSelected &operator=(const MenuItemNotSelected &other);

protected:
    enum ActiveStyle
    {
        STYLE_SIMPLE = 0,   // Center
        STYLE_TIP,          // Msg + tip
    };

    ActiveStyle mStyle { };

    TypedTextId mMsgId = {TYPED_TEXT_INVALID};
    TypedTextId mMsgIdType = {TYPED_TEXT_INVALID};

    TypedTextId mMsgIdTip = {TYPED_TEXT_INVALID};
    TypedTextId mMsgIdTipType = {TYPED_TEXT_INVALID};
    touchgfx::colortype mTipColor {};

    touchgfx::Image mIcon;

    void reset();
    void updStyle();
};

#endif // MENUITEMNOTSELECTED_HPP
