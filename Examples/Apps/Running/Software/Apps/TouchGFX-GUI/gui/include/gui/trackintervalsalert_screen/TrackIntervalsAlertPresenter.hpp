#ifndef TRACKINTERVALSALERTPRESENTER_HPP
#define TRACKINTERVALSALERTPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class TrackIntervalsAlertView;

class TrackIntervalsAlertPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    TrackIntervalsAlertPresenter(TrackIntervalsAlertView& v);

    virtual void activate();
    virtual void deactivate();

    virtual ~TrackIntervalsAlertPresenter() {}

    /// A new phase while this one shows: show it, for the full time again.
    virtual void onIntervalsPhaseAlert() override;

private:
    TrackIntervalsAlertPresenter();

    void showPhase();

    TrackIntervalsAlertView& view;
};

#endif // TRACKINTERVALSALERTPRESENTER_HPP
