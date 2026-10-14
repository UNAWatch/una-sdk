#include <gui/workoutlist_screen/WorkoutListView.hpp>
#include <gui/workoutlist_screen/WorkoutListPresenter.hpp>

WorkoutListPresenter::WorkoutListPresenter(WorkoutListView& v)
    : view(v)
{
}

void WorkoutListPresenter::activate()
{
    model->resetIdleTimer();
    model->menu().workouts.resetChildren();
    onWorkoutList();
}

void WorkoutListPresenter::onWorkoutList()
{
    view.setList(model->getWorkoutLibrary(), model->getArmedWorkout() != Model::kNoWorkout,
                 model->isUnitsImperial(), model->getTodayYmd());
    view.setPositionId(model->menu().workouts.get());
}

void WorkoutListPresenter::deactivate()
{
    model->menu().workouts.set(view.getPositionId());
}

void WorkoutListPresenter::chooseWorkout(uint16_t index)
{
    model->setChosenWorkout(index);
}

void WorkoutListPresenter::clearWorkout()
{
    model->armWorkout(Model::kNoWorkout);
    model->menu().workouts.reset();
}
