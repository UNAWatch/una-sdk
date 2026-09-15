/**
 ******************************************************************************
 * @file    TrackActionScreen.cpp
 * @brief   Paused-activity menu (see TrackActionScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/TrackActionScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Format.hpp"

namespace
{
constexpr uint32_t kCarouselPeriodMs = 3000;

using Style = WheelMenu::Item::Style;
const WheelMenu::Item kItems[App::MenuNav::TrackView::Action::ID_COUNT] = {
    { Style::Simple, "Resume" },
    { Style::Simple, "Summary" },
    { Style::Simple, "Save & End" },
    { Style::Simple, "Discard" },
};
} // namespace

TrackActionScreen::TrackActionScreen(Model& model)
    : Screen(model)
{
}

void TrackActionScreen::build()
{
    // Surrounding item text sits 6 px higher than the default (TrackActionView).
    mMenu    = std::make_unique<WheelMenu>(mRoot, kItems, Menu::ID_COUNT, -6);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::NONE);
    mPause    = std::make_unique<Widgets::PauseIndicator>(mRoot, 200);
    mCarousel = std::make_unique<Widgets::InfoCarousel>(mRoot, 40, 0);
    mCarousel->setPeriodMs(kCarouselPeriodMs);
    mCarousel->setCallback(&TrackActionScreen::carouselCb, this);
}

void TrackActionScreen::onShow()
{
    mIsImperial = mModel.isUnitsImperial();
    onTrackData(mModel.getTrackData());
    mModel.resetIdleTimer();
    mMenu->select(mModel.menu().track.action.get());
    mCarousel->setCount(4);
    mModel.trackPause();
}

void TrackActionScreen::onHide()
{
    mModel.menu().track.action.set(mMenu->selected());
}

void TrackActionScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;

        // Save & End / Discard start a hold-to-confirm the moment R1 goes down;
        // the countdown screen runs while R1 stays held.
        case Btn::R1_PRESS:
            if (mMenu->selected() == Menu::ID_SAVE) {
                mModel.setHoldConfirmMode(Model::HoldConfirmMode::Finish);
                ScreenManager::instance().goTo(ScreenId::TrackHoldConfirm);
            } else if (mMenu->selected() == Menu::ID_DISCARD) {
                mModel.setHoldConfirmMode(Model::HoldConfirmMode::Discard);
                ScreenManager::instance().goTo(ScreenId::TrackHoldConfirm);
            }
            break;

        case Btn::R1:
            if (mMenu->selected() == Menu::ID_RESUME) {
                mModel.trackResume();
                ScreenManager::instance().goTo(ScreenId::Track);
            } else if (mMenu->selected() == Menu::ID_SUMMARY) {
                ScreenManager::instance().goTo(ScreenId::TrackSummary);
            }
            break;

        default:
            break;
    }
}

void TrackActionScreen::onTrackData(const Track::Data& data)
{
    mPause->setTime(data.totalTime);
    mAvgPaceConv   = Fmt::paceUnits(data.avgPace, mIsImperial);
    mDistanceConv  = Fmt::distUnits(data.distance, mIsImperial);
    mAvgHr         = data.avgHR;
    mElevationConv = mIsImperial ? SDK::Utils::metersToFeet(data.elevation) : data.elevation;
    mCarousel->refresh();
}

void TrackActionScreen::carouselCb(void* user, int16_t index)
{
    static_cast<TrackActionScreen*>(user)->updateCarousel(index);
}

void TrackActionScreen::updateCarousel(int16_t index)
{
    char buf[16];
    switch (index) {
        case 0:
            mCarousel->setTitle("AVG. PACE");
            Fmt::pace(buf, sizeof(buf), mAvgPaceConv);
            break;
        case 1:
            mCarousel->setTitle("DISTANCE");
            Fmt::distanceTotal(buf, sizeof(buf), mDistanceConv);
            break;
        case 2:
            mCarousel->setTitle("AVG. HR");
            Fmt::heartRate(buf, sizeof(buf), mAvgHr);
            break;
        case 3:
            mCarousel->setTitle("ELEVATION");
            snprintf(buf, sizeof(buf), "%d", static_cast<int>(mElevationConv));
            break;
        default:
            return;
    }
    mCarousel->setValue(buf);
}
