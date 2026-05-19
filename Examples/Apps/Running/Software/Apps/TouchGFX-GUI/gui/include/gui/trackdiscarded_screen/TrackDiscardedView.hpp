#ifndef TRACKDISCARDEDVIEW_HPP
#define TRACKDISCARDEDVIEW_HPP

#include <gui_generated/trackdiscarded_screen/TrackDiscardedViewBase.hpp>
#include <gui/trackdiscarded_screen/TrackDiscardedPresenter.hpp>
#include <gui/containers/CountdownTimer.hpp>
#include <touchgfx/Callback.hpp>

class TrackDiscardedView : public TrackDiscardedViewBase
{
public:
    TrackDiscardedView();
    virtual ~TrackDiscardedView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

private:
    void onDismiss();

    CountdownTimer                          mDismissTimer;
    touchgfx::Callback<TrackDiscardedView>  mDismissCb;
};

#endif // TRACKDISCARDEDVIEW_HPP
