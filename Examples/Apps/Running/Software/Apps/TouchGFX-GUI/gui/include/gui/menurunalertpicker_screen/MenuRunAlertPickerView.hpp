#ifndef MENURUNALERTPICKERVIEW_HPP
#define MENURUNALERTPICKERVIEW_HPP

#include <gui_generated/menurunalertpicker_screen/MenuRunAlertPickerViewBase.hpp>
#include <gui/menurunalertpicker_screen/MenuRunAlertPickerPresenter.hpp>
#include <gui/containers/PickerLogic.hpp>

/**
 * The Run Alerts min:sec picker for the field being edited
 * (Model::runAlertField()): Run Time, Walk Time, Too Fast or Too Slow.
 * Bounds and the opening value come from RunAlerts::pickerBounds() and
 * pickerSeed(), so 0:00 and an inverted pace range can't be picked. Keys as
 * the Intervals pickers.
 */
class MenuRunAlertPickerView : public MenuRunAlertPickerViewBase
{
public:
    MenuRunAlertPickerView() {}
    virtual ~MenuRunAlertPickerView() {}
    virtual void setupScreen();

    /// Open on @p seconds within [@p lo, @p hi] for @p field.
    void setValue(RunAlerts::Field field, bool imperial, uint32_t seconds, uint32_t lo, uint32_t hi);

protected:
    void render();

    PickerLogic::BoundedTime mLogic;
    RunAlerts::Field         mField    = RunAlerts::Field::RunTime;
    bool                     mImperial = false;

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // MENURUNALERTPICKERVIEW_HPP
