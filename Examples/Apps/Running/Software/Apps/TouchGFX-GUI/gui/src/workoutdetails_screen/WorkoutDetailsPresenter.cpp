#include <gui/workoutdetails_screen/WorkoutDetailsView.hpp>
#include <gui/workoutdetails_screen/WorkoutDetailsPresenter.hpp>

WorkoutDetailsPresenter::WorkoutDetailsPresenter(WorkoutDetailsView& v)
    : view(v)
{
}

void WorkoutDetailsPresenter::activate()
{
    model->resetIdleTimer();
    const SDK::Workout::Library* lib = model->getWorkoutLibrary();
    const uint16_t chosen = model->getChosenWorkout();
    if (lib != nullptr && chosen < lib->size()) {
        view.setEntry(lib->entry(chosen), model->isUnitsImperial());
        // The steps follow once the service has read the file.
        model->requestWorkoutDetails(chosen);
    }
}

void WorkoutDetailsPresenter::deactivate()
{
}

void WorkoutDetailsPresenter::onWorkoutDetails()
{
    view.setProgram(model->getWorkoutDetails(), model->isUnitsImperial());
}

void WorkoutDetailsPresenter::back()
{
    if (model->isDetailsFromToday()) {
        model->application().gotoWorkoutTodayScreenNoTransition();
    } else {
        model->application().gotoWorkoutMenuScreenNoTransition();
    }
}
