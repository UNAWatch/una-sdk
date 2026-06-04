#ifndef MENUINTERVALSRUNDISTANCEVIEW_HPP
#define MENUINTERVALSRUNDISTANCEVIEW_HPP

#include <gui_generated/menuintervalsrundistance_screen/MenuIntervalsRunDistanceViewBase.hpp>
#include <gui/menuintervalsrundistance_screen/MenuIntervalsRunDistancePresenter.hpp>
#include <gui/containers/PickerLogic.hpp>

class MenuIntervalsRunDistanceView : public MenuIntervalsRunDistanceViewBase
{
public:
    MenuIntervalsRunDistanceView() {}
    virtual ~MenuIntervalsRunDistanceView() {}
    virtual void setupScreen();

    void setDistance(float meters, bool isImperial);

protected:
    using Menu = App::MenuNav::Root::Intervals::DistancePicker;

    PickerLogic::Distance<Menu> mLogic;

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // MENUINTERVALSRUNDISTANCEVIEW_HPP
