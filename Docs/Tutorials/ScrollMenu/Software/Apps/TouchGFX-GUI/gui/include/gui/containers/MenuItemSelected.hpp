#ifndef MENUITEMSELECTED_HPP
#define MENUITEMSELECTED_HPP

#include <gui_generated/containers/MenuItemSelectedBase.hpp>
#include <touchgfx/Unicode.hpp>

class MenuItemSelected : public MenuItemSelectedBase
{
public:
    MenuItemSelected();
    virtual ~MenuItemSelected() {}

    virtual void initialize();

    void config(TypedTextId msgId);
    void config(touchgfx::Unicode::UnicodeChar *msg);

    void configToggle(TypedTextId msgId);
    void configToggle(touchgfx::Unicode::UnicodeChar *msg);

    void configTip(TypedTextId msgIdLeft, TypedTextId msgIdRight);
    void configTip(TypedTextId msgId, touchgfx::Unicode::UnicodeChar *msgTip);

    void setMsgTypedTextId(TypedTextId msgIdType);
    void setTipTypedTextId(TypedTextId msgIdType);

    void setToggle(bool state);
    bool getToggle();

    MenuItemSelected &operator=(const MenuItemSelected &other);

protected:
    enum ActiveStyle
    {
        STYLE_SIMPLE = 0,   // Center
        STYLE_TOGGLE,       // Text + toggle
        STYLE_TIP,          // Msg + tip
    };

    ActiveStyle mStyle { };

    TypedTextId mMsgId = {TYPED_TEXT_INVALID};
    TypedTextId mMsgIdType = {TYPED_TEXT_INVALID};

    TypedTextId mMsgIdTip = {TYPED_TEXT_INVALID};
    TypedTextId mMsgIdTipType = {TYPED_TEXT_INVALID};

    void reset();
    void alignTextY(TextArea &t) const;
    void updStyle();

};

#endif // MENUITEMSELECTED_HPP
