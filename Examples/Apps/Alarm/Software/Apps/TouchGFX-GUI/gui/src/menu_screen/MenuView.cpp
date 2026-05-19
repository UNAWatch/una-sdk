#include <gui/menu_screen/MenuView.hpp>

MenuView::MenuView() :
    mUpdateItemCb(this,       &MenuView::updateItem),
    mUpdateCenterItemCb(this, &MenuView::updateCenterItem)
{
}

void MenuView::setupScreen()
{
    MenuViewBase::setupScreen();

    title.set(T_TEXT_ALARM_UC);

    buttons.setL1(Buttons::WHITE);
    buttons.setL2(Buttons::WHITE);
    buttons.setR1(Buttons::AMBER);
    buttons.setR2(Buttons::WHITE);

    menu.setUpdateItemCallback(mUpdateItemCb);
    menu.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menu.setNumberOfItems(Alarm::ACTION_COUNT);
    menu.invalidate();
}

void MenuView::tearDownScreen()
{
    MenuViewBase::tearDownScreen();
}

void MenuView::setAlarmState(size_t id, bool state)
{
    mId      = id;
    mAlarmOn = state;

    Unicode::snprintf(messageTextBuffer, MESSAGETEXT_SIZE, "%s %d", touchgfx::TypedText(T_TEXT_ALARM).getText(), id + 1);
    messageText.invalidate();

    menu.invalidate();
}

void MenuView::updateItem(OptionWheelItem& item, int16_t index)
{
    OptionWheelConfig cfg;
    switch (index) {
    case Alarm::ACTION_TOGGLE:  cfg.msgId = T_TEXT_OFF_ON; break;
    case Alarm::ACTION_EDIT:    cfg.msgId = T_TEXT_EDIT;   break;
    case Alarm::ACTION_DELETE:  cfg.msgId = T_TEXT_DELETE; break;
    default: return;
    }
    item.apply(cfg);
}

void MenuView::updateCenterItem(OptionWheelCenterItem& item, int16_t index)
{
    OptionWheelConfig cfg;
    switch (index) {
    case Alarm::ACTION_TOGGLE:
        cfg.style       = OptionWheelConfig::TOGGLE;
        cfg.msgIdLeft   = T_TEXT_OFF_UC;
        cfg.msgIdRight  = T_TEXT_ON_UC;
        cfg.toggleState = mAlarmOn;
        break;
    case Alarm::ACTION_EDIT:    cfg.msgId = T_TEXT_EDIT;   break;
    case Alarm::ACTION_DELETE:  cfg.msgId = T_TEXT_DELETE; break;
    default: return;
    }
    item.apply(cfg);
}

void MenuView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        menu.selectPrev();
    }

    if (key == SDK::GUI::Button::L2) {
        menu.selectNext();
    }

    if (key == SDK::GUI::Button::R1) {
        switch (menu.getSelectedItem()) {
        case Alarm::ACTION_TOGGLE:
            mAlarmOn = !mAlarmOn;
            menu.invalidate();
            presenter->save(mId, mAlarmOn);
            break;
        case Alarm::ACTION_EDIT:
            application().gotoEditScreenNoTransition();
            break;
        case Alarm::ACTION_DELETE:
            application().gotoDeletedScreenNoTransition();
            break;
        default:
            break;
        }
    }

    if (key == SDK::GUI::Button::R2) {
        application().gotoMainScreenNoTransition();
    }
}
