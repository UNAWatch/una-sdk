#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>
#include <Commands.hpp>

class MainView : public MainViewBase
{
public:
    MainView();
    virtual ~MainView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void updateSettingsDisplay(int32_t decimalCounter, CustomMessage::ActivityType activityType, CustomMessage::DisplayMode displayMode);

protected:
    virtual void handleKeyEvent(uint8_t key) override;

private:
    int32_t mDecimalCounter;
    CustomMessage::ActivityType mActivityType;
    CustomMessage::DisplayMode mDisplayMode;
    enum StgType {STG1,STG2,STG3,_MAX_STG};
    StgType stgSel;
};

#endif // MAINVIEW_HPP
