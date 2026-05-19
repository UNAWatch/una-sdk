#ifndef TRACKACTIONVIEW_HPP
#define TRACKACTIONVIEW_HPP

#include <gui_generated/trackaction_screen/TrackActionViewBase.hpp>
#include <gui/trackaction_screen/TrackActionPresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>

class TrackActionView : public TrackActionViewBase
{
public:
    TrackActionView();
    virtual ~TrackActionView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setPositionId(uint16_t id);
    uint16_t getPositionId();
    void setUnitsImperial(bool isImperial);
    void setTimer(std::time_t sec);
    void setAvgPace(float secPerM);
    void setDistance(float metres);
    void setAvgHR(float hr);
    void setElevation(float metres);
    void setAvgSpeed(float mPerSec);

protected:
    using Menu = App::MenuNav::TrackView::Action;

    bool mIsImperial = false;

    float mAvgPaceConv   = 0.0f;
    float mDistanceConv  = 0.0f;
    float mAvgHr         = 0.0f;
    float mElevationConv = 0.0f;
    float mSpeedConv     = 0.0f;

    ItemLayout mItemLayout{};

    touchgfx::Callback<TrackActionView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<TrackActionView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;
    touchgfx::Callback<TrackActionView, int16_t>                      mCarouselCb;

    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);
    void onCarouselUpdate(int16_t index);

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // TRACKACTIONVIEW_HPP
