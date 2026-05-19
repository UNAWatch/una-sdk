#include <gui/menudistancesaved_screen/MenuDistanceSavedView.hpp>
#include <gui/menudistancesaved_screen/MenuDistanceSavedPresenter.hpp>

MenuDistanceSavedPresenter::MenuDistanceSavedPresenter(MenuDistanceSavedView& v)
    : view(v)
{

}

void MenuDistanceSavedPresenter::activate()
{
    view.setDistanceUnits(model->getSettings().alertDistanceId, model->isUnitsImperial());
}

void MenuDistanceSavedPresenter::deactivate()
{

}
