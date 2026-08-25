#ifndef TRACKINTERVALSWORKOUTCOMPLETEDVIEW_HPP
#define TRACKINTERVALSWORKOUTCOMPLETEDVIEW_HPP

#include <gui_generated/trackintervalsworkoutcompleted_screen/TrackIntervalsWorkoutCompletedViewBase.hpp>
#include <gui/trackintervalsworkoutcompleted_screen/TrackIntervalsWorkoutCompletedPresenter.hpp>
#include <touchgfx/Callback.hpp>
#include <SDK/GUI/CountdownTimer.hpp>

class TrackIntervalsWorkoutCompletedView : public TrackIntervalsWorkoutCompletedViewBase
{
public:
    TrackIntervalsWorkoutCompletedView();
    virtual ~TrackIntervalsWorkoutCompletedView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

private:
    void onAutoAdvanceTimerFired();

    SDK::GUI::CountdownTimer                             mAutoAdvanceTimer;
    touchgfx::Callback<TrackIntervalsWorkoutCompletedView> mAutoAdvanceTimerCb;
};

#endif // TRACKINTERVALSWORKOUTCOMPLETEDVIEW_HPP
