#include <gui/deleted_screen/DeletedView.hpp>
#include <gui/deleted_screen/DeletedPresenter.hpp>

DeletedPresenter::DeletedPresenter(DeletedView& v)
    : view(v)
{
}

void DeletedPresenter::activate()
{
    model->resetIdleTimer();
}

void DeletedPresenter::deactivate()
{
}
