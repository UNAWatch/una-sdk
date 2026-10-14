#include <gui/main_screen/MainView.hpp>
#include <gui/main_screen/MainPresenter.hpp>

MainPresenter::MainPresenter(MainView& v)
    : view(v)
{

}

void MainPresenter::activate()
{
    view.setPositionId(model->menu().get());
    model->menu().resetChildren();
    model->resetIdleTimer();

    view.setGpsFix(model->hasGpsFix());
    view.setAccessoryStatus(model->getAccessoryState(), "");
    onWorkoutList();
}

void MainPresenter::onWorkoutList()
{
    const SDK::Workout::Library* lib = model->getWorkoutLibrary();
    const int16_t armed = model->getArmedWorkout();
    if (lib != nullptr && armed >= 0 && static_cast<size_t>(armed) < lib->size()) {
        view.setArmedWorkout(lib->entry(static_cast<size_t>(armed)).name);
    } else {
        view.setArmedWorkout(nullptr);
    }
}

void MainPresenter::checkTodayPrompt()
{
    if (model->getArmedWorkout() == Model::kNoWorkout && model->takeTodayPrompt()) {
        model->setChosenWorkout(model->getTodayWorkout());
        model->application().gotoWorkoutTodayScreenNoTransition();
    }
}

void MainPresenter::deactivate()
{
    model->menu().set(view.getPositionId());
}

void MainPresenter::onIdleTimeout()
{
    if (view.getPositionId() != App::MenuNav::Root::ID_START) {
        model->exitApp();
    }
}

void MainPresenter::onGpsFix(bool acquired)
{
    view.setGpsFix(acquired);
}

void MainPresenter::onAccessoryStatus(uint8_t state, const char* name)
{
    view.setAccessoryStatus(state, name);
}

void MainPresenter::startTrack()
{
    model->trackStart(false);
}

void MainPresenter::setPendingIntervalsMode(bool mode)
{
    model->setPendingIntervalsMode(mode);
}

void MainPresenter::exitApp()
{
    model->exitApp();
}
