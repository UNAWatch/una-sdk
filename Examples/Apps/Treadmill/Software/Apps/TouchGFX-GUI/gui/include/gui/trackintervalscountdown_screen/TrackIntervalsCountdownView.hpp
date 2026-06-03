#ifndef TRACKINTERVALSCOUNTDOWNVIEW_HPP
#define TRACKINTERVALSCOUNTDOWNVIEW_HPP

#include <gui_generated/trackintervalscountdown_screen/TrackIntervalsCountdownViewBase.hpp>
#include <gui/trackintervalscountdown_screen/TrackIntervalsCountdownPresenter.hpp>

class TrackIntervalsCountdownView : public TrackIntervalsCountdownViewBase
{
public:
    TrackIntervalsCountdownView();
    virtual ~TrackIntervalsCountdownView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setIntervals(const Settings::Intervals& inervals, bool isImperial);

protected:
    static const uint32_t kTimeoutSec = 5;
    uint32_t mLastDisplayedSecond = 0;

    virtual void handleKeyEvent(uint8_t key) override;
    void onConfirm();
    void updateTimerText(uint32_t seconds);

    touchgfx::Callback<TrackIntervalsCountdownView, int32_t> mTimerValueChangedCb;
    touchgfx::Callback<TrackIntervalsCountdownView, int32_t> mTimerCompleteCb;

    void onTimerValueChanged(int32_t value);
    void onTimerComplete(int32_t value);
};

#endif // TRACKINTERVALSCOUNTDOWNVIEW_HPP
