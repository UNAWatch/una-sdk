#ifndef MENURUNWALKPRESENTER_HPP
#define MENURUNWALKPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuRunWalkView;

class MenuRunWalkPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuRunWalkPresenter(MenuRunWalkView& v);

    virtual void activate();
    virtual void deactivate();

    virtual ~MenuRunWalkPresenter() {}

    virtual void onIdleTimeout() override { model->exitApp(); }
    virtual void onGpsFix(bool acquired) override;
    virtual void onAccessoryStatus(uint8_t state, const char* name) override;

    const Settings::RunAlerts& runAlerts() const;
    void toggleRunWalk();
    /// The picker is about to edit @p field.
    void edit(RunAlerts::Field field);

private:
    MenuRunWalkPresenter();

    MenuRunWalkView& view;
};

#endif // MENURUNWALKPRESENTER_HPP
