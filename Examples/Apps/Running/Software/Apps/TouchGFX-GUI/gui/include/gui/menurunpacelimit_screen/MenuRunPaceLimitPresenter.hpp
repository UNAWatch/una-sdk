#ifndef MENURUNPACELIMITPRESENTER_HPP
#define MENURUNPACELIMITPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuRunPaceLimitView;

class MenuRunPaceLimitPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuRunPaceLimitPresenter(MenuRunPaceLimitView& v);

    virtual void activate();
    virtual void deactivate();

    virtual ~MenuRunPaceLimitPresenter() {}

    virtual void onIdleTimeout() override { model->exitApp(); }
    virtual void onGpsFix(bool acquired) override;
    virtual void onAccessoryStatus(uint8_t state, const char* name) override;

    const Settings::RunAlerts& runAlerts() const;
    bool isImperial() const;
    RunAlerts::Field field() const;
    /// The other limit leaves room for one to be picked.
    bool canPick() const;
    void save(uint32_t value);

private:
    MenuRunPaceLimitPresenter();

    MenuRunPaceLimitView& view;
};

#endif // MENURUNPACELIMITPRESENTER_HPP
