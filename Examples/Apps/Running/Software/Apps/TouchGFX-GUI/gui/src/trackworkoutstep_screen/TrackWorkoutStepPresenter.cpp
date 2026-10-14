#include <gui/trackworkoutstep_screen/TrackWorkoutStepView.hpp>
#include <gui/trackworkoutstep_screen/TrackWorkoutStepPresenter.hpp>

TrackWorkoutStepPresenter::TrackWorkoutStepPresenter(TrackWorkoutStepView& v)
    : view(v)
{
}

void TrackWorkoutStepPresenter::activate()
{
    model->takeStepCard();
    view.setStep(model->getWorkoutStep(false));
}

void TrackWorkoutStepPresenter::deactivate()
{
}

void TrackWorkoutStepPresenter::onWorkoutStep(bool next)
{
    if (!next) {
        model->takeStepCard();
        view.setStep(model->getWorkoutStep(false));
    }
}

void TrackWorkoutStepPresenter::onIntervalsWorkoutCompleted()
{
    model->application().gotoTrackIntervalsWorkoutCompletedScreenNoTransition();
}
