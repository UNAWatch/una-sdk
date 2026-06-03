#ifndef MENUINTERVALSREPEATSPRESENTER_HPP
#define MENUINTERVALSREPEATSPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuIntervalsRepeatsView;

class MenuIntervalsRepeatsPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuIntervalsRepeatsPresenter(MenuIntervalsRepeatsView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    virtual ~MenuIntervalsRepeatsPresenter() {}

    virtual void onIdleTimeout() override { model->exitApp(); }

    void saveRepeats(uint8_t repeatsNum);

private:
    MenuIntervalsRepeatsPresenter();

    MenuIntervalsRepeatsView& view;
};

#endif // MENUINTERVALSREPEATSPRESENTER_HPP
