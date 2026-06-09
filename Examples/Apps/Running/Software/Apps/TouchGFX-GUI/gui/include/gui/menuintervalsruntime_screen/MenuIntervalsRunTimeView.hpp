#ifndef MENUINTERVALSRUNTIMEVIEW_HPP
#define MENUINTERVALSRUNTIMEVIEW_HPP

#include <gui_generated/menuintervalsruntime_screen/MenuIntervalsRunTimeViewBase.hpp>
#include <gui/menuintervalsruntime_screen/MenuIntervalsRunTimePresenter.hpp>
#include <gui/containers/PickerLogic.hpp>

class MenuIntervalsRunTimeView : public MenuIntervalsRunTimeViewBase
{
public:
    MenuIntervalsRunTimeView() {}
    virtual ~MenuIntervalsRunTimeView() {}
    virtual void setupScreen();

    void setTime(uint32_t totalSeconds);

protected:
    using Menu = App::MenuNav::Root::Intervals::TimePicker;

    PickerLogic::Time<Menu> mLogic;

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // MENUINTERVALSRUNTIMEVIEW_HPP
