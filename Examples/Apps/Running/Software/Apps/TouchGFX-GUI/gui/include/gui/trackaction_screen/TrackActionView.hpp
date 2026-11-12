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

    /// The selected item, by Menu id rather than row, as rows shift when
    /// an item is left out.
    void setPositionId(uint16_t id);
    uint16_t getPositionId();

    /// Shows the Lap item (and enables R2) when @p lap, and End workout
    /// while a structured workout runs.
    void setMode(bool lap, bool workout);
    void setUnitsImperial(bool isImperial);
    void setTimer(std::time_t sec);
    void setAvgPace(float secPerM);
    void setDistance(float metres);
    void setAvgHR(float hr);
    void setElevation(float metres);

protected:
    using Menu = App::MenuNav::TrackView::Action;

    bool mIsImperial = false;

    float mAvgPaceConv   = 0.0f; 
    float mDistanceConv  = 0.0f;
    float mAvgHr         = 0.0f;
    float mElevationConv = 0.0f;

    ItemLayout mItemLayout {};

    touchgfx::Callback<TrackActionView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<TrackActionView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;
    touchgfx::Callback<TrackActionView, int16_t>                      mCarouselCb;

    /// The item shown in row @p row.
    uint16_t itemAt(int16_t row) const;
    bool     mLapShown    = true;
    bool     mWorkoutMode = false;

    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);
    void onCarouselUpdate(int16_t index);

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // TRACKACTIONVIEW_HPP
