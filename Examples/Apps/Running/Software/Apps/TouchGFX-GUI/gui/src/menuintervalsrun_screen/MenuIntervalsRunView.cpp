#include <gui/menuintervalsrun_screen/MenuIntervalsRunView.hpp>

MenuIntervalsRunView::MenuIntervalsRunView() :
    mUpdateItemCb(this, &MenuIntervalsRunView::updateItem),
    mUpdateCenterItemCb(this, &MenuIntervalsRunView::updateCenterItem)
{

}

void MenuIntervalsRunView::setupScreen()
{
    MenuIntervalsRunViewBase::setupScreen();

    setupItems();
    menuLayout.setAnimationSteps(App::Config::kMenuAnimationSteps);
    menuLayout.setTitle(T_TEXT_RUN_UC);
    menuLayout.setUpdateItemCallback(mUpdateItemCb);
    menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menuLayout.setNumberOfItems(Menu::ID_COUNT);
    menuLayout.invalidate();
}

void MenuIntervalsRunView::tearDownScreen()
{
    MenuIntervalsRunViewBase::tearDownScreen();
}

void MenuIntervalsRunView::setupItems()
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

void MenuIntervalsRunView::setRunValue(const Settings::Intervals& iv, bool isImperial)
{
    // Time tip
    if (iv.runTime == 0) {
        Unicode::snprintf(mTimeTipBuff, kBuffSize, "%s", touchgfx::TypedText(T_TEXT_OPEN).getText());
    } else {
        auto hms = SDK::Utils::toHMS(iv.runTime);
        Unicode::snprintf(mTimeTipBuff, kBuffSize, "%02u:%02u %s",
            hms.m + (hms.h * 60), hms.s, touchgfx::TypedText(T_TEXT_MIN).getText());
    }

    // Distance tip
    if (iv.runDistance < 0.001f) {
        Unicode::snprintf(mDistanceTipBuff, kBuffSize, "%s", touchgfx::TypedText(T_TEXT_OPEN).getText());
    } else {
        float v = iv.runDistance / 1000.0f;
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

void MenuIntervalsRunView::updateItem(MainMenuItem& item, int16_t index)
{
    if (index < 0 || index >= Menu::ID_COUNT) {
        return;
    }
    item.apply(mItems[index]);
}

void MenuIntervalsRunView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    if (index < 0 || index >= Menu::ID_COUNT) {
        return;
    }
    item.apply(mCenterItems[index]);
}

void MenuIntervalsRunView::setPositionId(uint16_t id)
{
    menuLayout.selectItem(id);
}

uint16_t MenuIntervalsRunView::getPositionId()
{
    return menuLayout.getSelectedItem();
}

void MenuIntervalsRunView::handleKeyEvent(uint8_t key)
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

void MenuIntervalsRunView::onConfirm()
{
    switch (menuLayout.getSelectedItem()) {
    case Menu::ID_TIME:
        application().gotoMenuIntervalsRunTimeScreenNoTransition();
        break;
    case Menu::ID_DISTANCE:
        application().gotoMenuIntervalsRunDistanceScreenNoTransition();
        break;
    case Menu::ID_OPEN:
        presenter->saveOpenRun();
        application().gotoMenuIntervalsScreenNoTransition();
        break;
    default:
        break;
    }
}
