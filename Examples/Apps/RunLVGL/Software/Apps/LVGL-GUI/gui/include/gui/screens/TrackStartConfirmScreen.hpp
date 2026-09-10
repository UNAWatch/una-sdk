/**
 ******************************************************************************
 * @file    TrackStartConfirmScreen.hpp
 * @brief   "Start before signal acquired?" R1 starts anyway, R2 goes back.
 ******************************************************************************
 */

#ifndef TRACK_START_CONFIRM_SCREEN_HPP
#define TRACK_START_CONFIRM_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class TrackStartConfirmScreen : public Screen
{
public:
    explicit TrackStartConfirmScreen(Model& model);

    void onShow() override;
    void onKey(uint8_t code) override;
    void onIdleTimeout() override;

protected:
    void build() override;

private:
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // TRACK_START_CONFIRM_SCREEN_HPP
