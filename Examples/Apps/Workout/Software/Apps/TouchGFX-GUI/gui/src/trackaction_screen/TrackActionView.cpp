#include <gui/trackaction_screen/TrackActionView.hpp>

constexpr uint16_t kTrackTitleInfoSwitchPeriod = SDK::Utils::secToTicks(3, App::Config::kFrameRate);

TrackActionView::TrackActionView() :
    mUpdateItemCb(this, &TrackActionView::updateItem),
    mUpdateCenterItemCb(this, &TrackActionView::updateCenterItem),
    mCarouselCb(this, &TrackActionView::onCarouselUpdate)
{
}

void TrackActionView::setupScreen()
{
    TrackActionViewBase::setupScreen();

    menuLayout.showTitle(false);

    menuLayout.getButtons().setL1(Buttons::NONE);
    menuLayout.getButtons().setL2(Buttons::NONE);
    menuLayout.getButtons().setR1(Buttons::AMBER);
    menuLayout.getButtons().setR2(Buttons::NONE);

    menuLayout.setAnimationSteps(App::Config::kMenuAnimationSteps);
    menuLayout.setUpdateItemCallback(mUpdateItemCb);
    menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menuLayout.setNumberOfItems(Menu::ID_COUNT);

    mItemLayout.simple.msgOffsetY = -6;
    menuLayout.getMenu().setItemLayout(mItemLayout);

    infoCarousel.setPeriod(kTrackTitleInfoSwitchPeriod);
    infoCarousel.setUpdateCallback(mCarouselCb);
    infoCarousel.setCount(2);   // fires onCarouselUpdate(0) immediately

    menuLayout.invalidate();
}

void TrackActionView::tearDownScreen()
{
    TrackActionViewBase::tearDownScreen();
}

// ---- Presenter -> View ------------------------------------------------------

void TrackActionView::setPositionId(uint16_t id)
{
    menuLayout.selectItem(id);
}

uint16_t TrackActionView::getPositionId()
{
    return menuLayout.getSelectedItem();
}

void TrackActionView::setUnitsImperial(bool isImperial)
{
    mIsImperial = isImperial;
    infoCarousel.refresh();
}

void TrackActionView::setTimer(std::time_t sec)
{
    pauseIndicator.setTime(sec);
}

void TrackActionView::setCalories(float calories)
{
    mCalories = calories;
    infoCarousel.refresh();
}

void TrackActionView::setAvgHR(float hr)
{
    mAvgHr = hr;
    infoCarousel.refresh();
}

// ---- Menu callbacks --------------------------------------------------------

void TrackActionView::updateItem(MainMenuItem& item, int16_t index)
{
    static const TypedTextId sIds[Menu::ID_COUNT] = {
        T_TEXT_RESUME,
        T_TEXT_SUMMARY,
        T_TEXT_SAVE,
        T_TEXT_DISCARD,
    };

    if (index < 0 || index >= static_cast<int16_t>(Menu::ID_COUNT)) return;

    MenuItemConfig cfg;
    cfg.msgId = sIds[index];
    item.apply(cfg);
}

void TrackActionView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    static const TypedTextId sIds[Menu::ID_COUNT] = {
        T_TEXT_RESUME,
        T_TEXT_SUMMARY,
        T_TEXT_SAVE,
        T_TEXT_DISCARD,
    };

    if (index < 0 || index >= static_cast<int16_t>(Menu::ID_COUNT)) return;

    MenuItemConfig cfg;
    cfg.msgId = sIds[index];
    item.apply(cfg);
}

// ---- Carousel callback -----------------------------------------------------

void TrackActionView::onCarouselUpdate(int16_t index)
{
    const uint16_t kBufSize = 10;
    touchgfx::Unicode::UnicodeChar buf[kBufSize] {};

    switch (index) {

    case 0:
        infoCarousel.setTitle(T_TEXT_AVG_DOT_HR);
        if (mAvgHr < App::Display::kMinHR) {
            Unicode::snprintf(buf, kBufSize, "---");
        } else {
            Unicode::snprintfFloat(buf, kBufSize, "%.0f", mAvgHr);
        }
        break;

    case 1:
        infoCarousel.setTitle(T_TEXT_CALORIE_UC);
        Unicode::snprintfFloat(buf, kBufSize, "%.0f", mCalories);
        break;

    default:
        break;
    }

    infoCarousel.setValue(buf);
}

// ---- Input -----------------------------------------------------------------

void TrackActionView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        menuLayout.selectPrev();
        infoCarousel.refresh();  // tick visibility depends on selected item
    }

    if (key == SDK::GUI::Button::L2) {
        menuLayout.selectNext();
        infoCarousel.refresh();
    }

    // Save & End / Discard start a hold-to-confirm: pressing (and holding) R1 opens the
    // countdown screen immediately; it counts down while R1 stays held and releasing
    // early returns here. Resume/Summary stay plain taps.
    if (key == SDK::GUI::Button::R1_PRESS) {
        switch (menuLayout.getSelectedItem()) {
        case Menu::ID_SAVE:
            presenter->setHoldConfirmMode(Model::HoldConfirmMode::Finish);
            application().gotoTrackHoldConfirmationScreenNoTransition();
            break;
        case Menu::ID_DISCARD:
            presenter->setHoldConfirmMode(Model::HoldConfirmMode::Discard);
            application().gotoTrackHoldConfirmationScreenNoTransition();
            break;
        default:
            break;
        }
    }

    if (key == SDK::GUI::Button::R1) {
        switch (menuLayout.getSelectedItem()) {
        case Menu::ID_RESUME:
            presenter->resumeTrack();
            application().gotoTrackScreenNoTransition();
            break;
        case Menu::ID_SUMMARY:
            application().gotoTrackSummaryScreenNoTransition();
            break;
        default:
            break;
        }
    }
}
