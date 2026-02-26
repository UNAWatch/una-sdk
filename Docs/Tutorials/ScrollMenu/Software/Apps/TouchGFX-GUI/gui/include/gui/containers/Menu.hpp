#ifndef MENU_HPP
#define MENU_HPP

#include <gui_generated/containers/MenuBase.hpp>

class Menu : public MenuBase
{
public:
    Menu();
    virtual ~Menu() {}

    virtual void initialize();

    static const uint16_t skMaxNumberOfItems = 10;

    virtual void invalidate();

    void setNumberOfItems(int16_t numberOfItems);
    int16_t getNumberOfItems();

    void setTitle(TypedTextId msgId);
    void showTitle(bool state);

    void setBackground(BitmapId iconId);
    void showBackground(bool state);

    void setInfoMsg(TypedTextId msgId);
    void setInfoMsg(const char* msg);
    void setInfoMsgUnicode(const touchgfx::Unicode::UnicodeChar* msg);
    void setInfoMsgColor(touchgfx::colortype color);

    Buttons &getButtons();
    
    void selectNext();
    void selectPrev();
    void selectItem(int16_t itemIndex);
    uint16_t getSelectedItem();


    ScrollWheelWithSelectionStyle &getWheel();
    MenuItemSelected *getSelectedItem(int16_t itemIndex);
    MenuItemNotSelected *getNotSelectedItem(int16_t itemIndex);

protected:
    virtual void wheelUpdateItem(MenuItemNotSelected &item, int16_t itemIndex) override;
    virtual void wheelUpdateCenterItem(MenuItemSelected &item, int16_t itemIndex) override;

    touchgfx::Callback <Menu> mAnimationEndCb;
    void scrollWheelAnimationEndCb();

    MenuItemSelected mSelItems[skMaxNumberOfItems] { };
    MenuItemNotSelected mNotSelItems[skMaxNumberOfItems] { };
};

#endif // MENU_HPP
