#ifndef MENUPRESENTER_HPP
#define MENUPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuView;

/**
 * @brief Presenter for the Menu screen (Start / Edit / Delete).
 *
 * Feeds the view the value of the timer being acted on and, when Start is
 * chosen, launches the countdown for the current edit timer.
 */
class MenuPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuPresenter(MenuView& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~MenuPresenter() {}

    /** @brief Start the countdown for the current edit timer. */
    void startTimer();

    /** @brief Ask Main to re-select the edit timer when we return to it (R2). */
    void prepareReturnToMain() { model->requestRestoreSelection(model->editTimerIndex()); }

    /** @brief True when the timer being acted on is a preset (not deletable). */
    bool isEditPreset() const { return model->isPreset(model->getEditTimer()); }

private:
    MenuPresenter();

    MenuView& view;
};

#endif // MENUPRESENTER_HPP
