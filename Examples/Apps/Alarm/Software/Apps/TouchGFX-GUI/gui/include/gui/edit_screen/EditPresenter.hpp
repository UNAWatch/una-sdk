#ifndef EDITPRESENTER_HPP
#define EDITPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class EditView;

/**
 * @brief Presenter for the alarm edit screen.
 *
 * On activation loads the alarm being edited (or default values for a new alarm)
 * into the view. Persists changes to the model and navigates to the saved screen.
 */
class EditPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    EditPresenter(EditView& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~EditPresenter() {}

    virtual void onIdleTimeout() override { model->switchToNextPriorityScreen(); }

    /** @brief Cancel editing and return to the previous screen. */
    void exitScreen();

    /** @brief Persist the edited alarm and navigate to the saved confirmation screen. */
    void save(uint8_t h, uint8_t m, Alarm::Repeat repeat, Alarm::Effect effect);

private:
    EditPresenter();

    EditView& view;
};

#endif // EDITPRESENTER_HPP
