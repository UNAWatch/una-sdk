#include <gui/workoutmenu_screen/WorkoutMenuView.hpp>
#include <gui/workoutmenu_screen/WorkoutMenuPresenter.hpp>
#include <gui/common/WorkoutUi.hpp>

#include <cstdio>

WorkoutMenuPresenter::WorkoutMenuPresenter(WorkoutMenuView& v)
    : view(v)
{
}

void WorkoutMenuPresenter::activate()
{
    model->resetIdleTimer();
    const SDK::Workout::Library* lib = model->getWorkoutLibrary();
    const uint16_t chosen = model->getChosenWorkout();
    if (lib != nullptr && chosen < lib->size()) {
        const SDK::Workout::Library::Entry& e = lib->entry(chosen);
        // The title says when it is planned for, or that it is from the library.
        char group[16];
        if (e.date != 0 && e.date == model->getTodayYmd()) {
            touchgfx::Unicode::toUTF8(touchgfx::TypedText(T_TEXT_TODAY_UC).getText(),
                                      reinterpret_cast<uint8_t*>(group), sizeof(group));
        } else if (e.date != 0) {
            WorkoutUi::formatDate(e.date, group, sizeof(group));
        } else {
            touchgfx::Unicode::toUTF8(touchgfx::TypedText(T_TEXT_WORKOUTS_UC).getText(),
                                      reinterpret_cast<uint8_t*>(group), sizeof(group));
        }
        view.setWorkout(e, group, model->isUnitsImperial());
    }
    view.setPositionId(model->menu().workouts.menu.get());
}

void WorkoutMenuPresenter::deactivate()
{
    model->menu().workouts.menu.set(view.getPositionId());
}

void WorkoutMenuPresenter::start()
{
    model->armWorkout(static_cast<int16_t>(model->getChosenWorkout()));
    model->menu().set(App::MenuNav::Root::ID_START);
}

void WorkoutMenuPresenter::showDetails()
{
    model->setDetailsFromToday(false);
}
