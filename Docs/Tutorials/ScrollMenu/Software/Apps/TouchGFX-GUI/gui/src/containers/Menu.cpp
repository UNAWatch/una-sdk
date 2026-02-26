#include <gui/containers/Menu.hpp>

Menu::Menu() : 
    mAnimationEndCb(this, &Menu::scrollWheelAnimationEndCb)
{
    
}

void Menu::initialize()
{
    MenuBase::initialize();

    // Default buttons state
    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR1(Buttons::AMBER);
    buttons.setR2(Buttons::WHITE);

    for (int i = 0; i < skMaxNumberOfItems; i++) {
        mSelItems[i].initialize();
        mNotSelItems[i].initialize();
    }
    
    wheel.setAnimationEndedCallback(mAnimationEndCb);
}

void Menu::invalidate()
{
    for (int i = 0; i < wheel.getNumberOfItems(); i++) {
        wheel.itemChanged(i);
    }
    wheel.invalidate();
}

void Menu::setNumberOfItems(int16_t numberOfItems)
{
    if (numberOfItems <= skMaxNumberOfItems) {
        wheel.setNumberOfItems(numberOfItems);
        sideBar.setCount(numberOfItems);
    }
}

int16_t Menu::getNumberOfItems()
{
    return wheel.getNumberOfItems();
}

void Menu::setTitle(TypedTextId msgId)
{
    title.set(msgId);
}

void Menu::showTitle(bool state)
{
    title.setVisible(state);
    title.invalidate();
}

void Menu::setBackground(BitmapId iconId)
{
    backgroung.setBitmap(touchgfx::Bitmap(iconId));
    backgroung.invalidate();
}

void Menu::showBackground(bool state)
{
    backgroung.setVisible(state);
    backgroung.invalidate();
}

void Menu::setInfoMsg(TypedTextId msgId)
{
    if (msgId != TYPED_TEXT_INVALID) {
        Unicode::snprintf(infoBuffer, INFO_SIZE, "%s", touchgfx::TypedText(msgId).getText());
        info.setVisible(true);
    } else {
        info.setVisible(false);
    }
    info.invalidate();
}

void Menu::setInfoMsg(const char* msg)
{
    if (msg != nullptr) {
        Unicode::snprintf(infoBuffer, INFO_SIZE, "%s", msg);
        info.setVisible(true);
    } else {
        info.setVisible(false);
    }
    info.invalidate();
}

void Menu::setInfoMsgUnicode(const touchgfx::Unicode::UnicodeChar* msg)
{
    if (msg != nullptr) {
        info.setWildcard(msg);
        info.setVisible(true);
    } else {
        info.setVisible(false);
    }
    info.invalidate();
}

void Menu::setInfoMsgColor(touchgfx::colortype color)
{
    info.setColor(color);
    info.invalidate();
}

Buttons &Menu::getButtons()
{
    return buttons;
}

void Menu::selectNext()
{
    if (wheel.getNumberOfItems() <= 1) {
        return;
    }

    int16_t p = wheel.getSelectedItem() + 1;
    wheel.animateToItem(p, Gui::Config::kMenuAnimationSteps);
    sideBar.animateToId(p, Gui::Config::kMenuAnimationSteps);
}

void Menu::selectPrev()
{
    if (wheel.getNumberOfItems() <= 1) {
        return;
    }

    int16_t p = wheel.getSelectedItem() - 1;
    wheel.animateToItem(p, Gui::Config::kMenuAnimationSteps);
    sideBar.animateToId(p, Gui::Config::kMenuAnimationSteps);
}

void Menu::selectItem(int16_t itemIndex)
{
    if (wheel.getNumberOfItems() <= 1) {
        return;
    }

    wheel.animateToItem(itemIndex, 0);
    sideBar.setActiveId(wheel.getSelectedItem());
}

uint16_t Menu::getSelectedItem()
{
    return wheel.getSelectedItem();
}

ScrollWheelWithSelectionStyle &Menu::getWheel()
{
    return wheel;
}

MenuItemSelected *Menu::getSelectedItem(int16_t itemIndex)
{
    if (itemIndex < skMaxNumberOfItems) {
        return &mSelItems[itemIndex];
    }

    return nullptr;
}

MenuItemNotSelected *Menu::getNotSelectedItem(int16_t itemIndex)
{
    if (itemIndex < skMaxNumberOfItems) {
        return &mNotSelItems[itemIndex];
    }

    return nullptr;
}

void Menu::wheelUpdateItem(MenuItemNotSelected &item, int16_t itemIndex)
{
    item = mNotSelItems[itemIndex];
}

void Menu::wheelUpdateCenterItem(MenuItemSelected &item, int16_t itemIndex)
{
    item = mSelItems[itemIndex];
}

void Menu::scrollWheelAnimationEndCb()
{

}
