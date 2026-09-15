/**
 ******************************************************************************
 * @file    TrackStartConfirmScreen.cpp
 * @brief   "Start before signal acquired?" confirmation.
 ******************************************************************************
 */

#include "gui/screens/TrackStartConfirmScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Assets.hpp"
#include "gui/Strings.hpp"

TrackStartConfirmScreen::TrackStartConfirmScreen(Model& model)
    : Screen(model)
{
}

void TrackStartConfirmScreen::build()
{
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    Theme::label(mRoot, Theme::Font::SemiBold20, "Start before\nsignal acquired?", 35, 94, 170);
    Theme::imageTinted(mRoot, &img_tickgreen_22x17, 186, 60, SDK::GUI::Color::YELLOW_DARK);
    mTitle = std::make_unique<Widgets::Title>(mRoot, Strings::kAppNameUc);
}

void TrackStartConfirmScreen::onShow()
{
    mModel.resetIdleTimer();
}

void TrackStartConfirmScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    if (code == Btn::R1) {
        if (mModel.isPendingIntervalsMode()) {
            // Intervals were configured and Start chosen without a fix: confirming
            // goes straight to the countdown, which starts the track itself.
            ScreenManager::instance().goTo(ScreenId::TrackIntervalsCountdown);
        } else {
            mModel.trackStart(false);
            ScreenManager::instance().goTo(ScreenId::Track);
        }
    } else if (code == Btn::R2) {
        ScreenManager::instance().goTo(ScreenId::Main);
    }
}

void TrackStartConfirmScreen::onIdleTimeout()
{
    ScreenManager::instance().goTo(ScreenId::Main);
}
