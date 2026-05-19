#ifndef TRACKSAVEDVIEW_HPP
#define TRACKSAVEDVIEW_HPP

#include <gui_generated/tracksaved_screen/TrackSavedViewBase.hpp>
#include <gui/tracksaved_screen/TrackSavedPresenter.hpp>
#include <gui/containers/CountdownTimer.hpp>
#include <touchgfx/Callback.hpp>

class TrackSavedView : public TrackSavedViewBase
{
public:
    TrackSavedView();
    virtual ~TrackSavedView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

private:
    void onDismiss();

    CountdownTimer                      mDismissTimer;
    touchgfx::Callback<TrackSavedView>  mDismissCb;
};

#endif // TRACKSAVEDVIEW_HPP
