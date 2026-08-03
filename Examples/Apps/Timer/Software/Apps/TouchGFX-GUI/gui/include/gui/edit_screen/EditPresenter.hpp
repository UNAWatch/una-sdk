#ifndef EDITPRESENTER_HPP
#define EDITPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class EditView;

/**
 * @brief Presenter for the Edit (Set Timer) screen.
 *
 * Seeds the pickers from the model's edit timer and writes the chosen duration
 * back before the flow continues to the Alert screen.
 */
class EditPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    EditPresenter(EditView& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~EditPresenter() {}

    /** @brief Store the edited duration (seconds) on the model's edit timer. */
    void setDuration(uint16_t durationSec);

private:
    EditPresenter();

    EditView& view;
};

#endif // EDITPRESENTER_HPP
