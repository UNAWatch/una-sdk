#include <gui/saved_screen/SavedView.hpp>
#include <gui/saved_screen/SavedPresenter.hpp>

SavedPresenter::SavedPresenter(SavedView& v)
    : view(v)
{

}

void SavedPresenter::activate()
{
    // Reset idle timer
    model->resetIdleTimer();
    view.setAlarmId(model->getAlarmEditId());
}

void SavedPresenter::deactivate()
{

}

void SavedPresenter::exitScreen()
{
    model->switchToNextPriorityScreen();
}