#ifndef MENUINTERVALSRESTDISTANCEVIEW_HPP
#define MENUINTERVALSRESTDISTANCEVIEW_HPP

#include <gui_generated/menuintervalsrestdistance_screen/MenuIntervalsRestDistanceViewBase.hpp>
#include <gui/menuintervalsrestdistance_screen/MenuIntervalsRestDistancePresenter.hpp>
#include <gui/containers/PickerLogic.hpp>

class MenuIntervalsRestDistanceView : public MenuIntervalsRestDistanceViewBase
{
public:
    MenuIntervalsRestDistanceView() {}
    virtual ~MenuIntervalsRestDistanceView() {}
    virtual void setupScreen();

    void setDistance(float meters, bool isImperial);

protected:
    using Menu = App::MenuNav::Root::Intervals::DistancePicker;

    PickerLogic::Distance<Menu> mLogic;

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // MENUINTERVALSRESTDISTANCEVIEW_HPP
