#ifndef MENURUNLIMITSPRESENTER_HPP
#define MENURUNLIMITSPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuRunLimitsView;

class MenuRunLimitsPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuRunLimitsPresenter(MenuRunLimitsView& v);

    virtual void activate();
    virtual void deactivate();

    virtual ~MenuRunLimitsPresenter() {}

    virtual void onIdleTimeout() override { model->exitApp(); }
    virtual void onGpsFix(bool acquired) override;
    virtual void onAccessoryStatus(uint8_t state, const char* name) override;

    const Settings::RunAlerts& runAlerts() const;
    bool isImperial() const;
    RunAlerts::Field field() const;
    /// The next screen is about to edit @p field.
    void edit(RunAlerts::Field field);

private:
    MenuRunLimitsPresenter();

    MenuRunLimitsView& view;
};

#endif // MENURUNLIMITSPRESENTER_HPP
