#ifndef MENURUNALERTPICKERPRESENTER_HPP
#define MENURUNALERTPICKERPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuRunAlertPickerView;

class MenuRunAlertPickerPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuRunAlertPickerPresenter(MenuRunAlertPickerView& v);

    virtual void activate();
    virtual void deactivate();

    virtual ~MenuRunAlertPickerPresenter() {}

    virtual void onIdleTimeout() override { model->exitApp(); }

    void save(uint32_t value);

private:
    MenuRunAlertPickerPresenter();

    MenuRunAlertPickerView& view;
};

#endif // MENURUNALERTPICKERPRESENTER_HPP
