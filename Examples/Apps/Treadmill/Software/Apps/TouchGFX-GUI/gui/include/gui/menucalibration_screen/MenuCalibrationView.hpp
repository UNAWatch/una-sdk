#ifndef MENUCALIBRATIONVIEW_HPP
#define MENUCALIBRATIONVIEW_HPP

#include <gui_generated/menucalibration_screen/MenuCalibrationViewBase.hpp>
#include <gui/menucalibration_screen/MenuCalibrationPresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>

class MenuCalibrationView : public MenuCalibrationViewBase
{
public:
    MenuCalibrationView();
    virtual ~MenuCalibrationView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setPositionId(uint16_t id);
    uint16_t getPositionId();

protected:
    using Menu = App::MenuNav::Root::Settings::Calibration;

    touchgfx::Callback<MenuCalibrationView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<MenuCalibrationView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;

    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // MENUCALIBRATIONVIEW_HPP
