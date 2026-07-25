#ifndef MAINPRESENTER_HPP
#define MAINPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MainView;

/**
 * @brief Presenter for the Main screen -- skeleton placeholder.
 */
class MainPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MainPresenter(MainView& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~MainPresenter() {}

private:
    MainPresenter();

    MainView& view;
};

#endif // MAINPRESENTER_HPP
