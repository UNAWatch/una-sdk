#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>

#include "Commands.hpp"

class MainView : public MainViewBase
{
public:
    MainView();
    virtual ~MainView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent() override;

    /// Draw one navigation state: name, distance, bearing and status.
    void showNav(const CustomMessage::NavState& nav);

    /// Report the outcome of an R1 press.
    void showTargetSaved(bool saved, float latitude, float longitude);

protected:
    virtual void handleKeyEvent(uint8_t key) override;

private:
    /// Write the distance row, sizing the font to what the string needs.
    void setDistance(const char* text);
    void setStatus(const char* text);

    /// Ticks left showing the save confirmation before the status line resumes.
    int mNoticeTicks = 0;
};

#endif // MAINVIEW_HPP
