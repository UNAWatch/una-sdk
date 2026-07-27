#ifndef RUNNINGVIEW_HPP
#define RUNNINGVIEW_HPP

#include <gui_generated/running_screen/RunningViewBase.hpp>
#include <gui/running_screen/RunningPresenter.hpp>

/**
 * @brief Running screen: the active / paused countdown.
 *
 * Shows the remaining time (MM:SS) updated each tick. All other graphics are
 * static and simply shown/hidden by state: running shows a pause action on R1;
 * paused shows resume on R1 and a red stop on L1. Reset (L2) is always offered,
 * R2 returns to Main while the service keeps counting.
 */
class RunningView : public RunningViewBase
{
public:
    RunningView();
    virtual ~RunningView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent() override;

    /** @brief Re-sync icons and button colours to the current countdown state. */
    void onStateChanged();

protected:
    virtual void handleKeyEvent(uint8_t key) override;

private:
    void updateTime();
    void syncControls();
};

#endif // RUNNINGVIEW_HPP
