#include <gui/menuintervalsrest_screen/MenuIntervalsRestView.hpp>

MenuIntervalsRestView::MenuIntervalsRestView() :
    mUpdateItemCb(this, &MenuIntervalsRestView::updateItem),
    mUpdateCenterItemCb(this, &MenuIntervalsRestView::updateCenterItem)
{

}

void MenuIntervalsRestView::setupScreen()
{
    MenuIntervalsRestViewBase::setupScreen();

    setupItems();
    menuLayout.setAnimationSteps(App::Config::kMenuAnimationSteps);
    menuLayout.setTitle(T_TEXT_REST_UC);
    menuLayout.setUpdateItemCallback(mUpdateItemCb);
    menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menuLayout.setNumberOfItems(Menu::ID_COUNT);
    menuLayout.invalidate();
}

void MenuIntervalsRestView::tearDownScreen()
{
    MenuIntervalsRestViewBase::tearDownScreen();
}

void MenuIntervalsRestView::setupItems()
{
    // TIME
    mCenterItems[Menu::ID_TIME].style     = MenuItemConfig::TIP;
    mCenterItems[Menu::ID_TIME].msgIdType = T_TMP_SEMIBOLD_25;
    mCenterItems[Menu::ID_TIME].msgId     = T_TEXT_TIME;
    mCenterItems[Menu::ID_TIME].tipText   = mTimeTipBuff;

    mItems[Menu::ID_TIME].style   = MenuItemConfig::TIP;
    mItems[Menu::ID_TIME].msgId   = T_TEXT_TIME;
    mItems[Menu::ID_TIME].tipText = mTimeTipBuff;

    // DISTANCE
    mCenterItems[Menu::ID_DISTANCE].style     = MenuItemConfig::TIP;
    mCenterItems[Menu::ID_DISTANCE].msgIdType = T_TMP_SEMIBOLD_25;
    mCenterItems[Menu::ID_DISTANCE].msgId     = T_TEXT_DISTANCE;
    mCenterItems[Menu::ID_DISTANCE].tipText   = mDistanceTipBuff;

    mItems[Menu::ID_DISTANCE].style   = MenuItemConfig::TIP;
    mItems[Menu::ID_DISTANCE].msgId   = T_TEXT_DISTANCE;
    mItems[Menu::ID_DISTANCE].tipText = mDistanceTipBuff;

    // OPEN
    mCenterItems[Menu::ID_OPEN].style = MenuItemConfig::SIMPLE;
    mCenterItems[Menu::ID_OPEN].msgId = T_TEXT_OPEN;

    mItems[Menu::ID_OPEN].style = MenuItemConfig::SIMPLE;
    mItems[Menu::ID_OPEN].msgId = T_TEXT_OPEN;
}

void MenuIntervalsRestView::setRestValue(const Settings::Intervals& iv, bool isImperial)
{
    // Time tip
    if (iv.restTime == 0) {
        Unicode::snprintf(mTimeTipBuff, kBuffSize, "%s", touchgfx::TypedText(T_TEXT_OPEN).getText());
    } else {
        auto hms = SDK::Utils::toHMS(iv.restTime);
        Unicode::snprintf(mTimeTipBuff, kBuffSize, "%02u:%02u %s",
            hms.m + (hms.h * 60), hms.s, touchgfx::TypedText(T_TEXT_MIN).getText());
    }

    // Distance tip
    if (iv.restDistance < 0.001f) {
        Unicode::snprintf(mDistanceTipBuff, kBuffSize, "%s", touchgfx::TypedText(T_TEXT_OPEN).getText());
    } else {
        float v = iv.restDistance / 1000.0f;
        if (isImperial) {
            v = SDK::Utils::kmToMiles(v);
        }
        Unicode::snprintfFloat(mDistanceTipBuff, kBuffSize, "%02.02f", v);
        uint16_t len = Unicode::strlen(mDistanceTipBuff);
        Unicode::snprintf(&mDistanceTipBuff[len], kBuffSize - len, " %s",
            isImperial ? touchgfx::TypedText(T_TEXT_MI).getText()
                       : touchgfx::TypedText(T_TEXT_KM).getText());
    }

    menuLayout.invalidate();
}

void MenuIntervalsRestView::updateItem(MainMenuItem& item, int16_t index)
{
    if (index < 0 || index >= Menu::ID_COUNT) {
        return;
    }
    item.apply(mItems[index]);
}

void MenuIntervalsRestView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    if (index < 0 || index >= Menu::ID_COUNT) {
        return;
    }
    item.apply(mCenterItems[index]);
}

void MenuIntervalsRestView::setPositionId(uint16_t id)
{
    menuLayout.selectItem(id);
}

uint16_t MenuIntervalsRestView::getPositionId()
{
    return menuLayout.getSelectedItem();
}

void MenuIntervalsRestView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        menuLayout.selectPrev();
    }

    if (key == SDK::GUI::Button::L2) {
        menuLayout.selectNext();
    }

    if (key == SDK::GUI::Button::R1) {
        onConfirm();
    }

    if (key == SDK::GUI::Button::R2) {
        application().gotoMenuIntervalsScreenNoTransition();
    }
}

void MenuIntervalsRestView::onConfirm()
{
    switch (menuLayout.getSelectedItem()) {
    case Menu::ID_TIME:
        application().gotoMenuIntervalsRestTimeScreenNoTransition();
        break;
    case Menu::ID_DISTANCE:
        application().gotoMenuIntervalsRestDistanceScreenNoTransition();
        break;
    case Menu::ID_OPEN:
        presenter->saveOpenRest();
        application().gotoMenuIntervalsScreenNoTransition();
        break;
    default:
        break;
    }
}
