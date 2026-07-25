#ifndef MENUPRESENTER_HPP
#define MENUPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuView;

/**
 * @brief Presenter for the Menu screen -- skeleton placeholder.
 */
class MenuPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuPresenter(MenuView& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~MenuPresenter() {}

private:
    MenuPresenter();

    MenuView& view;
};

#endif // MENUPRESENTER_HPP
