#ifndef MENUINTERVALSRESTTIMEVIEW_HPP
#define MENUINTERVALSRESTTIMEVIEW_HPP

#include <gui_generated/menuintervalsresttime_screen/MenuIntervalsRestTimeViewBase.hpp>
#include <gui/menuintervalsresttime_screen/MenuIntervalsRestTimePresenter.hpp>
#include <gui/containers/PickerLogic.hpp>

class MenuIntervalsRestTimeView : public MenuIntervalsRestTimeViewBase
{
public:
    MenuIntervalsRestTimeView() {}
    virtual ~MenuIntervalsRestTimeView() {}
    virtual void setupScreen();

    void setTime(uint32_t totalSeconds);

protected:
    using Menu = App::MenuNav::Root::Intervals::TimePicker;

    PickerLogic::Time<Menu> mLogic;

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // MENUINTERVALSRESTTIMEVIEW_HPP
