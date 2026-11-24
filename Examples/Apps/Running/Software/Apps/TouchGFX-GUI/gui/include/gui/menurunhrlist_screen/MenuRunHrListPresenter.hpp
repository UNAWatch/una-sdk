#ifndef MENURUNHRLISTPRESENTER_HPP
#define MENURUNHRLISTPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuRunHrListView;

class MenuRunHrListPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuRunHrListPresenter(MenuRunHrListView& v);

    virtual void activate();
    virtual void deactivate();

    virtual ~MenuRunHrListPresenter() {}

    virtual void onIdleTimeout() override { model->exitApp(); }
    virtual void onGpsFix(bool acquired) override;
    virtual void onAccessoryStatus(uint8_t state, const char* name) override;

    void save(uint32_t value);

private:
    MenuRunHrListPresenter();

    MenuRunHrListView& view;
};

#endif // MENURUNHRLISTPRESENTER_HPP
