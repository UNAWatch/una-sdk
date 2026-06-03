#ifndef TRACKPRESENTER_HPP
#define TRACKPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class TrackView;

class TrackPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    TrackPresenter(TrackView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    virtual ~TrackPresenter() {}

    virtual void onTrackData(const Track::Data& data) override;
    virtual void onBatteryLevel(uint8_t lvl) override;
    virtual void onTime(uint8_t hour, uint8_t minute, uint8_t sec) override;
    virtual void onLapChanged(uint8_t lapEnd) override;
    virtual void onIntervalsPhaseAlert() override;
    virtual void onIntervalsWorkoutCompleted() override;
    virtual void onGpsFix(bool acquired) override;

    void saveLap();
    void intervalsNextPhase();

private:
    TrackPresenter();

    TrackView& view;
};

#endif // TRACKPRESENTER_HPP
