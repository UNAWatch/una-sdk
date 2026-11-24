#ifndef MENURUNALERTSPRESENTER_HPP
#define MENURUNALERTSPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuRunAlertsView;

class MenuRunAlertsPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuRunAlertsPresenter(MenuRunAlertsView& v);

    virtual void activate();
    virtual void deactivate();

    virtual ~MenuRunAlertsPresenter() {}

    virtual void onIdleTimeout() override { model->exitApp(); }
    virtual void onGpsFix(bool acquired) override;
    virtual void onAccessoryStatus(uint8_t state, const char* name) override;

    const Settings::RunAlerts& runAlerts() const;
    bool isImperial() const;
    void toggleBackInTap();
    /// Pace or Heart Rate opens on its upper limit (Too Fast / High).
    void openLimits(RunAlerts::Field field);

private:
    MenuRunAlertsPresenter();

    MenuRunAlertsView& view;
};

#endif // MENURUNALERTSPRESENTER_HPP
