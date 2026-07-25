#ifndef EDITPRESENTER_HPP
#define EDITPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class EditView;

/**
 * @brief Presenter for the Edit screen -- skeleton placeholder.
 */
class EditPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    EditPresenter(EditView& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~EditPresenter() {}

private:
    EditPresenter();

    EditView& view;
};

#endif // EDITPRESENTER_HPP
