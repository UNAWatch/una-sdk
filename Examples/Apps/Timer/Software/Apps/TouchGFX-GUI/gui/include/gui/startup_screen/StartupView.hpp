#ifndef STARTUPVIEW_HPP
#define STARTUPVIEW_HPP

#include <gui_generated/startup_screen/StartupViewBase.hpp>
#include <gui/startup_screen/StartupPresenter.hpp>

/**
 * @brief Blank holding screen shown at launch until the first timer state.
 *
 * The app opens here (not on Main) so the Model can route to Main / Running /
 * Fired once the service reports the initial state, instead of flashing Main's
 * "New" face for a frame before a cold-start countdown takes over. Renders
 * nothing but the background and takes no input.
 */
class StartupView : public StartupViewBase
{
public:
    StartupView();
    virtual ~StartupView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // STARTUPVIEW_HPP
