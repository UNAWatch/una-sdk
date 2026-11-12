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
    infoCarousel.setCount(3);   // fires onCarouselUpdate(0) immediately

    menuLayout.invalidate();
}

void TrackActionView::tearDownScreen()
{
    TrackActionViewBase::tearDownScreen();
}

// ---- Presenter -> View ------------------------------------------------------

void TrackActionView::setPositionId(uint16_t id)
{
    // A left-out item selects the row above it: Lap's selects Resume.
    int16_t row = 0;
    for (int16_t r = 1; r < menuLayout.getNumberOfItems() && itemAt(r) <= id; ++r) {
        row = r;
    }
    menuLayout.selectItem(row);
}

void TrackActionView::setLapShown(bool lap)
{
    mLapShown = lap;
    menuLayout.getButtons().setR2(lap ? Buttons::AMBER : Buttons::NONE);
    menuLayout.setNumberOfItems(lap ? Menu::ID_COUNT : Menu::ID_COUNT - 1);
    menuLayout.invalidate();
}

uint16_t TrackActionView::itemAt(int16_t row) const
{
    // Without Lap, its row is left out.
    if (!mLapShown && row >= Menu::ID_LAP) {
        return static_cast<uint16_t>(row + 1);
    }
    return static_cast<uint16_t>(row);
}

uint16_t TrackActionView::getPositionId()
{
    return itemAt(static_cast<int16_t>(menuLayout.getSelectedItem()));
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

void TrackActionView::setAvgPace(float speedMps)
{
    // Average speed in display units (km/h or mph) replaces avg pace.
    mAvgPaceConv = App::Display::speedToDisplay(speedMps, mIsImperial);
    infoCarousel.refresh();
}

void TrackActionView::setDistance(float metres)
{
    auto distConv = [this](float m) -> float {
        const float km = m / 1000.0f;
        return mIsImperial ? SDK::Utils::kmToMiles(km) : km;
    };

    mDistanceConv = distConv(metres);
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
        T_TEXT_LAP,
        T_TEXT_SUMMARY,
        T_TEXT_SAVE,
        T_TEXT_DISCARD,
    };

    if (index < 0 || index >= menuLayout.getNumberOfItems()) return;

    MenuItemConfig cfg;
    cfg.msgId = sIds[itemAt(index)];
    item.apply(cfg);
}

void TrackActionView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    static const TypedTextId sIds[Menu::ID_COUNT] = {
        T_TEXT_RESUME,
        T_TEXT_LAP,
        T_TEXT_SUMMARY,
        T_TEXT_SAVE,
        T_TEXT_DISCARD,
    };

    if (index < 0 || index >= menuLayout.getNumberOfItems()) return;

    MenuItemConfig cfg;
    cfg.msgId = sIds[itemAt(index)];
    item.apply(cfg);
}

// ---- Carousel callback -----------------------------------------------------

void TrackActionView::onCarouselUpdate(int16_t index)
{
    const uint16_t kBufSize = 10;
    touchgfx::Unicode::UnicodeChar buf[kBufSize] {};

    switch (index) {

    case 0:
        infoCarousel.setTitle(T_TEXT_AVG_DOT_PACE_UC);   // renders "AVG. SPEED"
        if (mAvgPaceConv < App::Display::kMinSpeed) {
            Unicode::snprintf(buf, kBufSize, "---");
        } else {
            Unicode::snprintfFloat(buf, kBufSize, "%.1f", mAvgPaceConv);
        }
        break;

    case 1:
        infoCarousel.setTitle(T_TEXT_DISTANCE_UC); 
        {
            if (mDistanceConv < App::Display::kMinDist) {
                Unicode::snprintf(buf, kBufSize, "---");
            } else if (mDistanceConv < 100.0f) {
                Unicode::snprintfFloat(buf, kBufSize, "%.02f", mDistanceConv);
            } else {
                Unicode::snprintfFloat(buf, kBufSize, "%.01f", mDistanceConv);
            }
        }
        break;

    case 2:
        infoCarousel.setTitle(T_TEXT_AVG_DOT_HR);
        if (mAvgHr < App::Display::kMinHR) {
            Unicode::snprintf(buf, kBufSize, "---");
        } else {
            Unicode::snprintfFloat(buf, kBufSize, "%.0f", mAvgHr);
        }
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

    // Discard starts a hold-to-confirm (press & hold R1) opening the countdown screen,
    // which counts down while R1 stays held; releasing early returns here. Save & End
    // and the other items stay plain taps (Save routes to Calibrate & Save).
    if (key == SDK::GUI::Button::R1_PRESS) {
        if (itemAt(static_cast<int16_t>(menuLayout.getSelectedItem())) == Menu::ID_DISCARD) {
            presenter->setHoldConfirmMode(Model::HoldConfirmMode::Discard);
            application().gotoTrackHoldConfirmationScreenNoTransition();
        }
    }

    if (key == SDK::GUI::Button::R1) {
        switch (itemAt(static_cast<int16_t>(menuLayout.getSelectedItem()))) {
        case Menu::ID_RESUME:
            presenter->resumeTrack();
            application().gotoTrackScreenNoTransition();
            break;
        case Menu::ID_LAP:
            presenter->saveLap();
            break;
        case Menu::ID_SUMMARY:
            application().gotoTrackSummaryScreenNoTransition();
            break;
        case Menu::ID_SAVE:
            presenter->saveRequested();
            break;
        default:
            break;
        }
    }

    // The lap button takes a lap here too, while Lap is shown. The lap screen
    // shows once the Service has taken it: it ignores a lap with nothing in it.
    if (key == SDK::GUI::Button::R2 && mLapShown) {
        presenter->saveLap();
    }
}
