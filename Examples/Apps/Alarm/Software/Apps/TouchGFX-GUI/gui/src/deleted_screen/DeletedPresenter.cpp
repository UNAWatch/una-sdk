#include <gui/deleted_screen/DeletedView.hpp>
#include <gui/deleted_screen/DeletedPresenter.hpp>

DeletedPresenter::DeletedPresenter(DeletedView& v)
    : view(v)
{

}

void DeletedPresenter::activate()
{
    // Reset idle timer
    model->resetIdleTimer();

    size_t id = model->getAlarmEditId();
    size_t size = model->getAlarmList().size();

    if (id < size) {
        model->deleteAlarm(id);
    }

    view.setAlarmId(id);

    if (id >= size) {
        id = size - 1;
    }
    else if (id > 0){
        id--;
    }
    model->setAlarmEditId(id);
}

void DeletedPresenter::deactivate()
{

}

void DeletedPresenter::exitScreen()
{
    model->switchToNextPriorityScreen();
}