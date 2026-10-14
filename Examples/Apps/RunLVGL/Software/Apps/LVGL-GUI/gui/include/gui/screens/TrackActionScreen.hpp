/**
 ******************************************************************************
 * @file    TrackActionScreen.hpp
 * @brief   Paused-activity menu: Resume / End workout / Summary / Save & End /
 *          Discard, with a rotating stats panel above and the paused time
 *          below. End workout shows only while a structured workout runs.
 *
 * Port of the Run app's TrackActionView/Presenter. Entering pauses the track.
 * End workout, Save & End and Discard start a hold-to-confirm on R1 press;
 * Resume and Summary act on a click.
 ******************************************************************************
 */

#ifndef TRACK_ACTION_SCREEN_HPP
#define TRACK_ACTION_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"

class TrackActionScreen : public Screen
{
public:
    explicit TrackActionScreen(Model& model);

    // Screen
    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;

    // ModelListener
    void onTrackData(const Track::Data& data) override;

protected:
    void build() override;

private:
    using Menu = App::MenuNav::TrackView::Action;

    static void carouselCb(void* user, int16_t index);
    void updateCarousel(int16_t index);
    /// The item shown in row @p row.
    uint16_t itemAt(uint16_t row) const;

    WheelMenu::Item mItems[Menu::ID_COUNT] {};
    bool  mWorkoutMode  = false;
    bool  mIsImperial   = false;
    float mAvgPaceConv  = 0.0f;
    float mDistanceConv = 0.0f;
    float mAvgHr        = 0.0f;
    float mElevationConv = 0.0f;

    std::unique_ptr<WheelMenu>               mMenu;
    std::unique_ptr<Widgets::Buttons>        mButtons;
    std::unique_ptr<Widgets::PauseIndicator> mPause;
    std::unique_ptr<Widgets::InfoCarousel>   mCarousel;
};

#endif // TRACK_ACTION_SCREEN_HPP
