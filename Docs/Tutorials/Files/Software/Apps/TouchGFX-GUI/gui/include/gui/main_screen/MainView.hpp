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

    void updateSettingsDisplay(float decimalCounter, CustomMessage::ActivityType activityType, CustomMessage::DisplayMode displayMode);

protected:
    virtual void handleKeyEvent(uint8_t key) override;

private:
    float mDecimalCounter;
    CustomMessage::ActivityType mActivityType;
    CustomMessage::DisplayMode mDisplayMode;
    enum class StgType {STG1,STG2,STG3};
    StgType stgSel;
};

#endif // MAINVIEW_HPP
