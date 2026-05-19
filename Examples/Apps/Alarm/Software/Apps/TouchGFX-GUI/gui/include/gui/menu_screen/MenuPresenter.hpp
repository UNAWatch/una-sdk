#ifndef MENUPRESENTER_HPP
#define MENUPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuView;

/**
 * @brief Presenter for the alarm action menu screen.
 *
 * On activation loads the current on/off state of the selected alarm into the view.
 * Handles toggle, edit, and delete actions.
 */
class MenuPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuPresenter(MenuView& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~MenuPresenter() {}

    virtual void onIdleTimeout() override { model->switchToNextPriorityScreen(); }

    /** @brief Called when the alarm list changes so the view can refresh its toggle state. */
    virtual void onAlarmListUpdated(const std::vector<Alarm>& list) override;

    /** @brief Persist the new on/off @p state for alarm @p id. */
    void save(size_t id, bool state);

private:
    MenuPresenter();

    MenuView& view;
};

#endif // MENUPRESENTER_HPP
