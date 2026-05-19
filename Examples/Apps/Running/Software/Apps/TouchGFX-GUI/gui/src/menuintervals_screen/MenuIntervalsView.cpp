#include <gui/menuintervals_screen/MenuIntervalsView.hpp>

MenuIntervalsView::MenuIntervalsView() :
    mUpdateItemCb(this, &MenuIntervalsView::updateItem),
    mUpdateCenterItemCb(this, &MenuIntervalsView::updateCenterItem)
{

}

void MenuIntervalsView::setupScreen()
{
    MenuIntervalsViewBase::setupScreen();

    setupItems();
    menuLayout.setAnimationSteps(App::Config::kMenuAnimationSteps);
    menuLayout.setTitle(T_TEXT_INTERVALS_UC);
    menuLayout.setUpdateItemCallback(mUpdateItemCb);
    menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menuLayout.setNumberOfItems(Menu::ID_COUNT);
    menuLayout.invalidate();
}

void MenuIntervalsView::tearDownScreen()
{
    MenuIntervalsViewBase::tearDownScreen();
}

void MenuIntervalsView::setIntervals(const Settings::Intervals& inervals, bool isImperial)
{
    // Repeats
    if (inervals.repeatsNum == 0) {
        Unicode::snprintf(mRepeatsTipBuff, kBuffSize, "%s", touchgfx::TypedText(T_TEXT_OPEN).getText());
    } else {
        Unicode::snprintf(mRepeatsTipBuff, kBuffSize, "x%u", inervals.repeatsNum);
    }

    // Run
    if (inervals.runMetric == Settings::Intervals::OPEN) {
        Unicode::snprintf(mRunTipBuff, kBuffSize, "%s", touchgfx::TypedText(T_TEXT_OPEN).getText());
    } else if (inervals.runMetric == Settings::Intervals::DISTANCE) {
        if (inervals.runDistance < 0.001f) {
            Unicode::snprintf(mRunTipBuff, kBuffSize, "%s", touchgfx::TypedText(T_TEXT_OPEN).getText());
        } else {
            float v = inervals.runDistance / 1000.0f; // km
            if (isImperial) {
                v = SDK::Utils::kmToMiles(v); // mi
            }
            Unicode::snprintfFloat(mRunTipBuff, kBuffSize, "%02.02f", v);
            uint16_t len = Unicode::strlen(mRunTipBuff);
            Unicode::snprintf(&mRunTipBuff[len], kBuffSize - len, " %s",
                isImperial ? touchgfx::TypedText(T_TEXT_MI).getText() : touchgfx::TypedText(T_TEXT_KM).getText());
        }
    } else {
        if (inervals.runTime == 0) {
            Unicode::snprintf(mRunTipBuff, kBuffSize, "%s", touchgfx::TypedText(T_TEXT_OPEN).getText());
        } else {
            auto hms = SDK::Utils::toHMS(inervals.runTime);
            Unicode::snprintf(mRunTipBuff, kBuffSize, "%02u:%02u %s", hms.m + (hms.h * 60), hms.s, touchgfx::TypedText(T_TEXT_MIN).getText());
        }
    }

    // Rest
    if (inervals.restMetric == Settings::Intervals::OPEN) {
        Unicode::snprintf(mRestTipBuff, kBuffSize, "%s", touchgfx::TypedText(T_TEXT_OPEN).getText());
    } else if (inervals.restMetric == Settings::Intervals::DISTANCE) {
        if (inervals.restDistance < 0.001f) {
            Unicode::snprintf(mRestTipBuff, kBuffSize, "%s", touchgfx::TypedText(T_TEXT_OPEN).getText());
        } else {
            float v = inervals.restDistance / 1000.0f; // km
            if (isImperial) {
                v = SDK::Utils::kmToMiles(v); // mi
            }
            Unicode::snprintfFloat(mRestTipBuff, kBuffSize, "%02.02f", v);
            uint16_t len = Unicode::strlen(mRestTipBuff);
            Unicode::snprintf(&mRestTipBuff[len], kBuffSize - len, " %s",
                isImperial ? touchgfx::TypedText(T_TEXT_MI).getText() : touchgfx::TypedText(T_TEXT_KM).getText());
        }
    } else {
        if (inervals.restTime == 0) {
            Unicode::snprintf(mRestTipBuff, kBuffSize, "%s", touchgfx::TypedText(T_TEXT_OPEN).getText());
        } else {
            auto hms = SDK::Utils::toHMS(inervals.restTime);
            Unicode::snprintf(mRestTipBuff, kBuffSize, "%02u:%02u %s", hms.m + (hms.h * 60), hms.s, touchgfx::TypedText(T_TEXT_MIN).getText());
        }
    }

    // Warm Up
    updateToggle(Menu::ID_WARM_UP, inervals.warmUp);

    // Cool Down
    updateToggle(Menu::ID_COOL_DOWN, inervals.coolDown);

    menuLayout.invalidate();
}

void MenuIntervalsView::setPositionId(uint16_t id)
{
    menuLayout.selectItem(id);
}

uint16_t MenuIntervalsView::getPositionId()
{
    return menuLayout.getSelectedItem();
}

void MenuIntervalsView::setupItems()
{
    // START
    mCenterItems[Menu::ID_START].msgId = T_TEXT_START;
    mCenterItems[Menu::ID_START].msgIdType = T_TMP_SEMIBOLD_35;

    mItems[Menu::ID_START].msgId = T_TEXT_START;


    // REPEATS
#if 0 // New style
    static CenterItemLayout repeatsCenterLayout{};
    repeatsCenterLayout.inl.mainX = 37;
    repeatsCenterLayout.inl.mainW = 130;
    repeatsCenterLayout.inl.tipX  = 170;
    repeatsCenterLayout.inl.tipW  = 70;

    static ItemLayout repeatsLayout{};
    repeatsLayout.inl.mainX = 60;
    repeatsLayout.inl.mainW = 90;
    repeatsLayout.inl.tipX  = 150;
    repeatsLayout.inl.tipW  = 70;

    mCenterItems[Menu::ID_REPEATS].style = MenuItemConfig::INLINE;
    mCenterItems[Menu::ID_REPEATS].msgIdType = T_TMP_SEMIBOLD_30_L;
    mCenterItems[Menu::ID_REPEATS].msgId = T_TEXT_REPEATS;

    mCenterItems[Menu::ID_REPEATS].tipText = mRepeatsTipBuff;
    mCenterItems[Menu::ID_REPEATS].tipIdType = T_TMP_MEDIUM_18_L;
    mCenterItems[Menu::ID_REPEATS].tipColor = SDK::GUI::Color::WHITE;
    mCenterItems[Menu::ID_REPEATS].centerLayout = &repeatsCenterLayout;

    mItems[Menu::ID_REPEATS].style = MenuItemConfig::INLINE;
    mItems[Menu::ID_REPEATS].msgId = T_TEXT_REPEATS;

    mItems[Menu::ID_REPEATS].tipIdType = T_TMP_MEDIUM_18_L;
    mItems[Menu::ID_REPEATS].tipText = mRepeatsTipBuff;
    mItems[Menu::ID_REPEATS].tipColor = SDK::GUI::Color::WHITE;
    mItems[Menu::ID_REPEATS].itemLayout = &repeatsLayout;
#else
    mCenterItems[Menu::ID_REPEATS].style = MenuItemConfig::TIP;
    mCenterItems[Menu::ID_REPEATS].msgIdType = T_TMP_SEMIBOLD_25;
    mCenterItems[Menu::ID_REPEATS].msgId = T_TEXT_REPEATS;
    mCenterItems[Menu::ID_REPEATS].tipText = mRepeatsTipBuff;

    mItems[Menu::ID_REPEATS].style = MenuItemConfig::TIP;
    mItems[Menu::ID_REPEATS].msgId = T_TEXT_REPEATS;
    mItems[Menu::ID_REPEATS].tipText = mRepeatsTipBuff;
#endif


#if 0 // New style
    // RUN / REST layout
    static CenterItemLayout runRestCenterLayout{};
    runRestCenterLayout.inl.mainX = 37;
    runRestCenterLayout.inl.mainW = 80;
    runRestCenterLayout.inl.tipX  = 120;
    runRestCenterLayout.inl.tipW  = 100;

    static ItemLayout runRestLayout{};
    runRestLayout.inl.mainX = 70;
    runRestLayout.inl.mainW = 50;
    runRestLayout.inl.tipX  = 120;
    runRestLayout.inl.tipW  = 100;

    // RUN
    mCenterItems[Menu::ID_RUN].style = MenuItemConfig::INLINE;
    mCenterItems[Menu::ID_RUN].msgIdType = T_TMP_SEMIBOLD_30_L;
    mCenterItems[Menu::ID_RUN].msgId = T_TEXT_RUN;
    mCenterItems[Menu::ID_RUN].tipText = mRunTipBuff;
    mCenterItems[Menu::ID_RUN].tipIdType = T_TMP_MEDIUM_18_L;
    mCenterItems[Menu::ID_RUN].tipColor = SDK::GUI::Color::WHITE;
    mCenterItems[Menu::ID_RUN].centerLayout = &runRestCenterLayout;

    mItems[Menu::ID_RUN].style = MenuItemConfig::INLINE;
    mItems[Menu::ID_RUN].msgIdType = T_TMP_MEDIUM_18_L;
    mItems[Menu::ID_RUN].msgId = T_TEXT_RUN;
    mItems[Menu::ID_RUN].tipText = mRunTipBuff;
    mItems[Menu::ID_RUN].tipIdType = T_TMP_MEDIUM_10_L;
    mItems[Menu::ID_RUN].tipColor = SDK::GUI::Color::WHITE;
    mItems[Menu::ID_RUN].itemLayout = &runRestLayout;

    // REST
    mCenterItems[Menu::ID_REST].style = MenuItemConfig::INLINE;
    mCenterItems[Menu::ID_REST].msgIdType = T_TMP_SEMIBOLD_30_L;
    mCenterItems[Menu::ID_REST].msgId = T_TEXT_REST;
    mCenterItems[Menu::ID_REST].tipText = mRestTipBuff;
    mCenterItems[Menu::ID_REST].tipIdType = T_TMP_MEDIUM_18_L;
    mCenterItems[Menu::ID_REST].tipColor = SDK::GUI::Color::WHITE;
    mCenterItems[Menu::ID_REST].centerLayout = &runRestCenterLayout;

    mItems[Menu::ID_REST].style = MenuItemConfig::INLINE;
    mItems[Menu::ID_REST].msgIdType = T_TMP_MEDIUM_18_L;
    mItems[Menu::ID_REST].msgId = T_TEXT_REST;
    mItems[Menu::ID_REST].tipText = mRestTipBuff;
    mItems[Menu::ID_REST].tipIdType = T_TMP_MEDIUM_10_L;
    mItems[Menu::ID_REST].tipColor = SDK::GUI::Color::WHITE;
    mItems[Menu::ID_REST].itemLayout = &runRestLayout;
#else
    // RUN
    mCenterItems[Menu::ID_RUN].style = MenuItemConfig::TIP;
    mCenterItems[Menu::ID_RUN].msgIdType = T_TMP_SEMIBOLD_25;
    mCenterItems[Menu::ID_RUN].msgId = T_TEXT_RUN;
    mCenterItems[Menu::ID_RUN].tipText = mRunTipBuff;

    mItems[Menu::ID_RUN].style = MenuItemConfig::TIP;
    mItems[Menu::ID_RUN].msgId = T_TEXT_RUN;
    mItems[Menu::ID_RUN].tipText = mRunTipBuff;

    // REST
    mCenterItems[Menu::ID_REST].style = MenuItemConfig::TIP;
    mCenterItems[Menu::ID_REST].msgIdType = T_TMP_SEMIBOLD_25;
    mCenterItems[Menu::ID_REST].msgId = T_TEXT_REST;
    mCenterItems[Menu::ID_REST].tipText = mRestTipBuff;

    mItems[Menu::ID_REST].style = MenuItemConfig::TIP;
    mItems[Menu::ID_REST].msgId = T_TEXT_REST;
    mItems[Menu::ID_REST].tipText = mRestTipBuff;
#endif

    // WARM_UP
    mCenterItems[Menu::ID_WARM_UP].style = MenuItemConfig::TOGGLE;
    mCenterItems[Menu::ID_WARM_UP].msgIdType = T_TMP_SEMIBOLD_25;
    mCenterItems[Menu::ID_WARM_UP].msgId = T_TEXT_WARM_UP;
    mCenterItems[Menu::ID_WARM_UP].toggleState = false;

    mItems[Menu::ID_WARM_UP].style = MenuItemConfig::TIP;
    mItems[Menu::ID_WARM_UP].msgId = T_TEXT_WARM_UP;
    mItems[Menu::ID_WARM_UP].tipColor = SDK::GUI::Color::TEAL;
    mItems[Menu::ID_WARM_UP].tipId = T_TEXT_OFF_UC;

    // COOL_DOWN
    mCenterItems[Menu::ID_COOL_DOWN].style = MenuItemConfig::TOGGLE;
    mCenterItems[Menu::ID_COOL_DOWN].msgIdType = T_TMP_SEMIBOLD_25;
    mCenterItems[Menu::ID_COOL_DOWN].msgId = T_TEXT_COOL_BR_DOWN;
    mCenterItems[Menu::ID_COOL_DOWN].toggleState = false;

    mItems[Menu::ID_COOL_DOWN].style = MenuItemConfig::TIP;
    mItems[Menu::ID_COOL_DOWN].msgId = T_TEXT_COOL_DOWN;
    mItems[Menu::ID_COOL_DOWN].tipColor = SDK::GUI::Color::TEAL;
    mItems[Menu::ID_COOL_DOWN].tipId = T_TEXT_OFF_UC;

}

void MenuIntervalsView::updateItem(MainMenuItem& item, int16_t index)
{
    if (index < 0 || index >= Menu::ID_COUNT) {
        return;
    }
    item.apply(mItems[index]);
}

void MenuIntervalsView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    if (index < 0 || index >= Menu::ID_COUNT) {
        return;
    }
    item.apply(mCenterItems[index]);
}

void MenuIntervalsView::handleKeyEvent(uint8_t key)
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
        application().gotoMainScreenNoTransition();
    }
}

void MenuIntervalsView::onConfirm()
{
    int16_t idx = static_cast<int16_t>(menuLayout.getSelectedItem());
    if (idx < 0 || idx >= Menu::ID_COUNT) {
        return;
    }

    switch (idx) {
    case Menu::ID_START:
        application().gotoTrackIntervalsCountdownScreenNoTransition();
        break;

    case Menu::ID_REPEATS:
        application().gotoMenuIntervalsRepeatsScreenNoTransition();
        break;

    case Menu::ID_RUN:
        application().gotoMenuIntervalsRunScreenNoTransition();
        break;

    case Menu::ID_REST:
        application().gotoMenuIntervalsRestScreenNoTransition();
        break;

    case Menu::ID_WARM_UP:
    case Menu::ID_COOL_DOWN: {
        bool newState = !mCenterItems[idx].toggleState;
        updateToggle(idx, newState);
        menuLayout.invalidate();

        // Save settings
        if (idx == Menu::ID_WARM_UP) {
            presenter->saveWarmUp(newState);
        } else {
            presenter->saveCoolDown(newState);
        }
    } break;

    default:
        break;
    };
}

void MenuIntervalsView::updateToggle(int16_t index, bool newState)
{
    MenuItemConfig& centerCfg = mCenterItems[index];
    MenuItemConfig& itemCfg = mItems[index];

    centerCfg.toggleState = newState;
    itemCfg.toggleState = newState;
    itemCfg.tipColor = newState ? SDK::GUI::Color::YELLOW_DARK : SDK::GUI::Color::TEAL;
    itemCfg.tipId = newState ? T_TEXT_ON_UC : T_TEXT_OFF_UC;
}
