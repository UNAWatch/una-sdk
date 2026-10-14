#ifndef TRACKWORKOUTSTEPPRESENTER_HPP
#define TRACKWORKOUTSTEPPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class TrackWorkoutStepView;

class TrackWorkoutStepPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    TrackWorkoutStepPresenter(TrackWorkoutStepView& v);

    virtual void activate();
    virtual void deactivate();

    virtual ~TrackWorkoutStepPresenter() {}

    /// Another step started while the card showed: show that one.
    virtual void onWorkoutStep(bool next) override;
    virtual void onIntervalsWorkoutCompleted() override;

private:
    TrackWorkoutStepPresenter();

    TrackWorkoutStepView& view;
};

#endif // TRACKWORKOUTSTEPPRESENTER_HPP
