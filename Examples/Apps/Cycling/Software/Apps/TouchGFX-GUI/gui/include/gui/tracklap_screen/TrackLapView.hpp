#ifndef TRACKLAPVIEW_HPP
#define TRACKLAPVIEW_HPP

#include <gui_generated/tracklap_screen/TrackLapViewBase.hpp>
#include <gui/tracklap_screen/TrackLapPresenter.hpp>
#include <gui/containers/CountdownTimer.hpp>
#include <touchgfx/Callback.hpp>

class TrackLapView : public TrackLapViewBase
{
public:
    TrackLapView();
    virtual ~TrackLapView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setUnitsImperial(bool isImperial);
    void setLapNum(uint32_t n);
    void setDistance(float m);
    void setTimer(std::time_t sec);
    void setSpeed(float mPerSec);

protected:
    virtual void handleKeyEvent(uint8_t key) override;

private:
    void onDismiss();

    bool mIsImperial = false;

    CountdownTimer                      mDismissTimer;
    touchgfx::Callback<TrackLapView>    mDismissCb;
};

#endif // TRACKLAPVIEW_HPP
