#include <gui/menuintervalsrestdistance_screen/MenuIntervalsRestDistanceView.hpp>

MenuIntervalsRestDistanceView::MenuIntervalsRestDistanceView() :
    mUpdateItemCb(this, &MenuIntervalsRestDistanceView::updateItem),
    mUpdateCenterItemCb(this, &MenuIntervalsRestDistanceView::updateCenterItem)
{
    // Main text position is fixed for both stages
    mCenterLayoutWhole.inl.mainX = 70;
    mCenterLayoutWhole.inl.mainW = 100;
    mCenterLayoutFrac.inl.mainX  = 70;
    mCenterLayoutFrac.inl.mainW  = 100;

    // Stage WHOLE/FRAC: unit label (km / mi) on RIGHT
    mCenterLayoutWhole.inl.tipX = 170;
    mCenterLayoutWhole.inl.tipW = 40;
    mCenterLayoutFrac.inl.tipX = 170;
    mCenterLayoutFrac.inl.tipW = 40;
}

void MenuIntervalsRestDistanceView::setupScreen()
{
    MenuIntervalsRestDistanceViewBase::setupScreen();

    menuLayout.setAnimationSteps(App::Config::kMenuAnimationSteps);
    menuLayout.setTitle(T_TEXT_DISTANCE_UC);
    menuLayout.setUpdateItemCallback(mUpdateItemCb);
    menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menuLayout.setNumberOfItems(Menu::kCountWholeKm);
    menuLayout.invalidate();
}

void MenuIntervalsRestDistanceView::tearDownScreen()
{
    MenuIntervalsRestDistanceViewBase::tearDownScreen();
}

void MenuIntervalsRestDistanceView::setDistance(float meters, bool isImperial)
{
    mIsImperial = isImperial;

    float units = meters / 1000.0f;
    if (isImperial) units = SDK::Utils::kmToMiles(units);

    uint16_t maxWhole = isImperial ? Menu::kMaxWholeMi : Menu::kMaxWholeKm;
    mWhole = static_cast<uint16_t>(units);
    if (mWhole > maxWhole) mWhole = maxWhole;

    float frac = units - mWhole;
    uint16_t fracIdx = static_cast<uint16_t>(frac / Menu::kFracStep + 0.5f);
    if (fracIdx >= Menu::kCountFrac) fracIdx = Menu::kCountFrac - 1;
    mFracIdx = fracIdx;

    mStage = STAGE_WHOLE;
    uint16_t count = isImperial ? Menu::kCountWholeMi : Menu::kCountWholeKm;
    menuLayout.setNumberOfItems(count);
    menuLayout.selectItem(mWhole);
    menuLayout.invalidate();
}

void MenuIntervalsRestDistanceView::enterStage(Stage stage)
{
    mStage = stage;
    if (stage == STAGE_WHOLE) {
        uint16_t count = mIsImperial ? Menu::kCountWholeMi : Menu::kCountWholeKm;
        menuLayout.setNumberOfItems(count);
        menuLayout.selectItem(mWhole);
    } else {
        menuLayout.setNumberOfItems(Menu::kCountFrac);
        menuLayout.selectItem(mFracIdx);
    }
    menuLayout.invalidate();
}

void MenuIntervalsRestDistanceView::formatValue(
    touchgfx::Unicode::UnicodeChar* buf, uint16_t whole, uint16_t fracIdx)
{
    Unicode::snprintf(buf, kBuffSize, "%02u.%02u", whole, fracIdx * 5u);
}

void MenuIntervalsRestDistanceView::updateItem(MainMenuItem& item, int16_t index)
{
    if (mStage == STAGE_WHOLE) {
        formatValue(mItemBuff, static_cast<uint16_t>(index), mFracIdx);
    } else {
        formatValue(mItemBuff, mWhole, static_cast<uint16_t>(index));
    }

    MenuItemConfig cfg;
    cfg.style   = MenuItemConfig::SIMPLE;
    cfg.msgText = mItemBuff;
    item.apply(cfg);
}

void MenuIntervalsRestDistanceView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    if (mStage == STAGE_WHOLE) {
        formatValue(mMainBuff, static_cast<uint16_t>(index), mFracIdx);
    } else {
        formatValue(mMainBuff, mWhole, static_cast<uint16_t>(index));
    }

    MenuItemConfig cfg;
    cfg.style   = MenuItemConfig::INLINE;
    cfg.msgText = mMainBuff;

    if (mStage == STAGE_WHOLE) {
        cfg.tipId        = mIsImperial ? T_TEXT_MI : T_TEXT_KM;
        cfg.centerLayout = &mCenterLayoutWhole;
    } else {
        cfg.tipId        = mIsImperial ? T_TEXT_MI : T_TEXT_KM; // Use same units
        cfg.centerLayout = &mCenterLayoutFrac;
    }
    item.apply(cfg);
}

void MenuIntervalsRestDistanceView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        menuLayout.selectPrev();
    }

    if (key == SDK::GUI::Button::L2) {
        menuLayout.selectNext();
    }

    if (key == SDK::GUI::Button::R1) {
        if (mStage == STAGE_WHOLE) {
            mWhole = static_cast<uint16_t>(menuLayout.getSelectedItem());
            enterStage(STAGE_FRAC);
        } else {
            mFracIdx = static_cast<uint16_t>(menuLayout.getSelectedItem());
            float units = mWhole + mFracIdx * Menu::kFracStep;
            float km    = mIsImperial ? SDK::Utils::milesToKm(units) : units;
            presenter->saveRestDistance(km * 1000.0f);
            application().gotoMenuIntervalsScreenNoTransition();
        }
    }

    if (key == SDK::GUI::Button::R2) {
        if (mStage == STAGE_WHOLE) {
            application().gotoMenuIntervalsRestScreenNoTransition();
        } else {
            mFracIdx = static_cast<uint16_t>(menuLayout.getSelectedItem());
            enterStage(STAGE_WHOLE);
        }
    }
}
