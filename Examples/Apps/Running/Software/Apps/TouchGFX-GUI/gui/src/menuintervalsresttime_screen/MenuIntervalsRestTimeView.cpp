#include <gui/menuintervalsresttime_screen/MenuIntervalsRestTimeView.hpp>

MenuIntervalsRestTimeView::MenuIntervalsRestTimeView() :
    mUpdateItemCb(this, &MenuIntervalsRestTimeView::updateItem),
    mUpdateCenterItemCb(this, &MenuIntervalsRestTimeView::updateCenterItem)
{
    // Main text position is fixed for both stages
    mCenterLayoutMin.inl.mainX = 70;
    mCenterLayoutMin.inl.mainW = 100;
    mCenterLayoutSec.inl.mainX = 70;
    mCenterLayoutSec.inl.mainW = 100;

    // Stage MIN: "min" label to the LEFT of main text
    mCenterLayoutMin.inl.tipX  = 30;
    mCenterLayoutMin.inl.tipW  = 40;

    // Stage SEC: "sec" label to the RIGHT of main text
    mCenterLayoutSec.inl.tipX  = 170;
    mCenterLayoutSec.inl.tipW  = 40;
}

void MenuIntervalsRestTimeView::setupScreen()
{
    MenuIntervalsRestTimeViewBase::setupScreen();

    menuLayout.setAnimationSteps(App::Config::kMenuAnimationSteps);
    menuLayout.setTitle(T_TEXT_TIME_UC);
    menuLayout.setUpdateItemCallback(mUpdateItemCb);
    menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    // start in STAGE_MIN -- setTime() will selectItem after activate()
    menuLayout.setNumberOfItems(Menu::kCountMin);
    menuLayout.invalidate();
}

void MenuIntervalsRestTimeView::tearDownScreen()
{
    MenuIntervalsRestTimeViewBase::tearDownScreen();
}

void MenuIntervalsRestTimeView::setTime(uint32_t totalSeconds)
{
    uint16_t minutes = static_cast<uint16_t>(totalSeconds / 60u);
    uint16_t seconds = static_cast<uint16_t>(totalSeconds % 60u);

    if (minutes > Menu::kMaxMin) minutes = Menu::kMaxMin;

    seconds = ((seconds + Menu::kStepSec / 2u) / Menu::kStepSec) * Menu::kStepSec;
    if (seconds > Menu::kMaxSec) seconds = Menu::kMaxSec;

    mMinutes = minutes;
    mSeconds = seconds;
    mStage   = STAGE_MIN;
    menuLayout.selectItem(mMinutes / Menu::kStepMin);
    menuLayout.invalidate();
}

void MenuIntervalsRestTimeView::enterStage(Stage stage)
{
    mStage = stage;
    if (stage == STAGE_MIN) {
        menuLayout.setNumberOfItems(Menu::kCountMin);
        menuLayout.selectItem(mMinutes / Menu::kStepMin);
    } else {
        menuLayout.setNumberOfItems(Menu::kCountSec);
        menuLayout.selectItem(mSeconds / Menu::kStepSec);
    }
    menuLayout.invalidate();
}

void MenuIntervalsRestTimeView::updateItem(MainMenuItem& item, int16_t index)
{
    uint16_t minutes, seconds;
    if (mStage == STAGE_MIN) {
        minutes = static_cast<uint16_t>(index) * Menu::kStepMin;
        seconds = mSeconds;
    } else {
        minutes = mMinutes;
        seconds = static_cast<uint16_t>(index) * Menu::kStepSec;
    }
    Unicode::snprintf(mItemBuff, kBuffSize, "%02u:%02u", minutes, seconds);

    MenuItemConfig cfg;
    cfg.style   = MenuItemConfig::SIMPLE;
    cfg.msgText = mItemBuff;
    item.apply(cfg);
}

void MenuIntervalsRestTimeView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    uint16_t minutes, seconds;
    if (mStage == STAGE_MIN) {
        minutes = static_cast<uint16_t>(index) * Menu::kStepMin;
        seconds = mSeconds;
    } else {
        minutes = mMinutes;
        seconds = static_cast<uint16_t>(index) * Menu::kStepSec;
    }
    Unicode::snprintf(mMainBuff, kBuffSize, "%02u:%02u", minutes, seconds);

    MenuItemConfig cfg;
    cfg.style   = MenuItemConfig::INLINE;
    cfg.msgText = mMainBuff;
    if (mStage == STAGE_MIN) {
        cfg.tipId        = T_TEXT_MIN;
        cfg.centerLayout = &mCenterLayoutMin;
    } else {
        cfg.tipId        = T_TEXT_SEC;
        cfg.centerLayout = &mCenterLayoutSec;
    }
    item.apply(cfg);
}

void MenuIntervalsRestTimeView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        menuLayout.selectPrev();
    }

    if (key == SDK::GUI::Button::L2) {
        menuLayout.selectNext();
    }

    if (key == SDK::GUI::Button::R1) {
        if (mStage == STAGE_MIN) {
            mMinutes = menuLayout.getSelectedItem() * Menu::kStepMin;
            enterStage(STAGE_SEC);
        } else {
            mSeconds = menuLayout.getSelectedItem() * Menu::kStepSec;
            presenter->saveRestTime(mMinutes * 60u + mSeconds);
            application().gotoMenuIntervalsScreenNoTransition();
        }
    }

    if (key == SDK::GUI::Button::R2) {
        if (mStage == STAGE_MIN) {
            application().gotoMenuIntervalsRestScreenNoTransition();
        } else {
            mSeconds = menuLayout.getSelectedItem() * Menu::kStepSec;
            enterStage(STAGE_MIN);
        }
    }
}
