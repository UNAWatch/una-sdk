#ifndef RUNNINGVIEW_HPP
#define RUNNINGVIEW_HPP

#include <gui_generated/running_screen/RunningViewBase.hpp>
#include <gui/running_screen/RunningPresenter.hpp>

/**
 * @brief Running screen -- skeleton placeholder.
 *
 * Renders the app title and the screen name. Real widgets and logic are
 * added in the next stage.
 */
class RunningView : public RunningViewBase
{
public:
    RunningView() {}
    virtual ~RunningView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
};

#endif // RUNNINGVIEW_HPP
