#include <gui/edit_screen/EditView.hpp>
#include <gui/edit_screen/EditPresenter.hpp>

EditPresenter::EditPresenter(EditView& v)
    : view(v)
{
}

void EditPresenter::activate()
{
    model->resetIdleTimer();
    view.set(model->getEditTimer().durationSec);
}

void EditPresenter::deactivate()
{
}

void EditPresenter::setDuration(uint16_t durationSec)
{
    Timer t = model->getEditTimer();
    t.durationSec = durationSec;
    model->setEditTimer(t);
}
