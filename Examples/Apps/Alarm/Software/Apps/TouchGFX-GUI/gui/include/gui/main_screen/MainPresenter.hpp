#ifndef MAINPRESENTER_HPP
#define MAINPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MainView;

/**
 * @brief Presenter for the main alarm list screen.
 *
 * On activation loads the current alarm list and the last selected alarm index
 * into the view. Handles idle timeout by routing to the next priority screen.
 */
class MainPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MainPresenter(MainView& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~MainPresenter() {}

    virtual void onIdleTimeout() override { model->switchToNextPriorityScreen(); }

    /** @brief Called when the alarm list changes (e.g. after save/delete). */
    virtual void onAlarmListUpdated(const std::vector<Alarm>& list) override;

    /** @brief Store @p id as the alarm to be edited/actioned on the next screen. */
    void setAlarmEditId(size_t id);

    void exitApp();

private:
    MainPresenter();

    MainView& view;
};

#endif // MAINPRESENTER_HPP
