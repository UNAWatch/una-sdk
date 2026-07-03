#ifndef MENUINTERVALSRESTPRESENTER_HPP
#define MENUINTERVALSRESTPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuIntervalsRestView;

class MenuIntervalsRestPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuIntervalsRestPresenter(MenuIntervalsRestView& v);

    virtual void activate();
    virtual void deactivate();

    void saveOpenRest();

    virtual ~MenuIntervalsRestPresenter() {}

    virtual void onIdleTimeout() override { model->exitApp(); }
    virtual void onGpsFix(bool acquired) override;
    virtual void onAccessoryStatus(uint8_t state, const char* name) override;

private:
    MenuIntervalsRestPresenter();

    MenuIntervalsRestView& view;
};

#endif // MENUINTERVALSRESTPRESENTER_HPP
