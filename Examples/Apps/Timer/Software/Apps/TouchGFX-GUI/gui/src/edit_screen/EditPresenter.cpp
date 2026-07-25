#include <gui/edit_screen/EditView.hpp>
#include <gui/edit_screen/EditPresenter.hpp>

EditPresenter::EditPresenter(EditView& v)
    : view(v)
{
}

void EditPresenter::activate()
{
    model->resetIdleTimer();
}

void EditPresenter::deactivate()
{
}
