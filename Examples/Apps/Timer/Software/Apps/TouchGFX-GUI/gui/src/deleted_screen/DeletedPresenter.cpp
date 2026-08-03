#include <gui/deleted_screen/DeletedView.hpp>
#include <gui/deleted_screen/DeletedPresenter.hpp>

DeletedPresenter::DeletedPresenter(DeletedView& v)
    : view(v)
{
}

void DeletedPresenter::activate()
{
    model->resetIdleTimer();

    // Land Main on the timer that takes the freed slot -- the next one -- so
    // capture that slot before the removal shrinks the list.
    model->requestRestoreSelection(model->editTimerIndex());

    // Committing the delete here (rather than in Menu) keeps the action and its
    // confirmation together. A preset is not in recents, so this is a no-op for
    // it -- the screen still confirms.
    model->removeRecent(model->getEditTimer());
}

void DeletedPresenter::deactivate()
{
}
