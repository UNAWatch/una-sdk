/**
 ******************************************************************************
 * @file    TrackDetailScreen.hpp
 * @brief   Settings > Tracks > Track 2: when it was last used, how many runs,
 *          and Forget, held to confirm as RunLVGL's Discard is (R1 held). The
 *          other tracks keep their numbers. R2 goes back to the list.
 ******************************************************************************
 */

#ifndef TRACK_DETAIL_SCREEN_HPP
#define TRACK_DETAIL_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"

class TrackDetailScreen : public Screen
{
public:
    explicit TrackDetailScreen(Model& model);

    void onShow() override;
    void onKey(uint8_t code) override;
    void onIdleTimeout() override { mModel.exitApp(); }

protected:
    void build() override;

private:
    WheelMenu::Item mItem {};
    char            mTitleText[16] {};
    lv_obj_t*       mUsed = nullptr;
    lv_obj_t*       mRuns = nullptr;

    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
    std::unique_ptr<WheelMenu>        mMenu;
};

#endif // TRACK_DETAIL_SCREEN_HPP
