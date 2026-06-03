#include <gui/menutimesaved_screen/MenuTimeSavedView.hpp>
#include <gui/menutimesaved_screen/MenuTimeSavedPresenter.hpp>

MenuTimeSavedPresenter::MenuTimeSavedPresenter(MenuTimeSavedView& v)
    : view(v)
{

}

void MenuTimeSavedPresenter::activate()
{
    view.setTime(model->getSettings().alertTimeId);
}

void MenuTimeSavedPresenter::deactivate()
{

}
