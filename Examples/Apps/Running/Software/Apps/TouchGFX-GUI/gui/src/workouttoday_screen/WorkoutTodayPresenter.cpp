#include <gui/workouttoday_screen/WorkoutTodayView.hpp>
#include <gui/workouttoday_screen/WorkoutTodayPresenter.hpp>

WorkoutTodayPresenter::WorkoutTodayPresenter(WorkoutTodayView& v)
    : view(v)
{
}

void WorkoutTodayPresenter::activate()
{
    model->resetIdleTimer();
    const SDK::Workout::Library* lib = model->getWorkoutLibrary();
    const uint16_t today = model->getTodayWorkout();
    if (lib != nullptr && today < lib->size()) {
        view.setWorkout(lib->entry(today), model->isUnitsImperial());
    }
    view.setPositionId(model->menu().workouts.menu.get());
}

void WorkoutTodayPresenter::deactivate()
{
    model->menu().workouts.menu.set(view.getPositionId());
}

void WorkoutTodayPresenter::start()
{
    model->armWorkout(static_cast<int16_t>(model->getTodayWorkout()));
    model->menu().set(App::MenuNav::Root::ID_START);
}

void WorkoutTodayPresenter::showDetails()
{
    model->setChosenWorkout(model->getTodayWorkout());
    model->setDetailsFromToday(true);
}
