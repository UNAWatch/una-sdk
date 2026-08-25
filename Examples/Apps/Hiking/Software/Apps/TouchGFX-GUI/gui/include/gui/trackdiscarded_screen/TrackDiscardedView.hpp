#ifndef TRACKDISCARDEDVIEW_HPP
#define TRACKDISCARDEDVIEW_HPP

#include <gui_generated/trackdiscarded_screen/TrackDiscardedViewBase.hpp>
#include <gui/trackdiscarded_screen/TrackDiscardedPresenter.hpp>
#include <touchgfx/Callback.hpp>
#include <SDK/GUI/CountdownTimer.hpp>

class TrackDiscardedView : public TrackDiscardedViewBase
{
public:
    TrackDiscardedView();
    virtual ~TrackDiscardedView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

private:
    void onDismiss();

    SDK::GUI::CountdownTimer                mDismissTimer;
    touchgfx::Callback<TrackDiscardedView>  mDismissCb;
};

#endif // TRACKDISCARDEDVIEW_HPP
