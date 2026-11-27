/**
 ******************************************************************************
 * @file    TracksScreen.hpp
 * @brief   Settings > Tracks: the stored tracks, by number, each with the
 *          date it was last used and its use count ("12 Oct, 5 runs"), from
 *          the service's TrackList. "No tracks yet" when there are none.
 *          R1 opens a track (where it can be forgotten), R2 goes back.
 ******************************************************************************
 */

#ifndef TRACKS_SCREEN_HPP
#define TRACKS_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"

class TracksScreen : public Screen
{
public:
    explicit TracksScreen(Model& model);

    void onShow() override;
    void onKey(uint8_t code) override;
    void onIdleTimeout() override { mModel.exitApp(); }
    void onTrackList() override;

protected:
    void build() override;

private:
    static constexpr uint8_t kMax = CustomMessage::TrackList::kMax;

    void fill();

    WheelMenu::Item mItems[kMax] {};
    char            mTexts[kMax][12] {};
    char            mTips[kMax][24] {};
    uint8_t         mCount = 0;

    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
    lv_obj_t*                         mWheelBox = nullptr; ///< The wheel's parent, deleted with it
    lv_obj_t*                         mEmpty[3] {};        ///< "No tracks yet" and its two lines
    std::unique_ptr<WheelMenu>        mMenu;
};

#endif // TRACKS_SCREEN_HPP
