#ifndef SAVEDPRESENTER_HPP
#define SAVEDPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class SavedView;

/**
 * @brief Presenter for the "alarm saved" confirmation screen.
 *
 * Shows the index of the just-saved alarm and routes away via
 * switchToNextPriorityScreen() when the auto-dismiss timer fires or the
 * user navigates back.
 */
class SavedPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    SavedPresenter(SavedView& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~SavedPresenter() {}

    /** @brief Navigate away from this confirmation screen. */
    void exitScreen();

private:
    SavedPresenter();

    SavedView& view;
};

#endif // SAVEDPRESENTER_HPP
