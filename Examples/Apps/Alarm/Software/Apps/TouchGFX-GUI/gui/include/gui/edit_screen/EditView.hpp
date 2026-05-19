#ifndef EDITVIEW_HPP
#define EDITVIEW_HPP

#include <gui_generated/edit_screen/EditViewBase.hpp>
#include <gui/edit_screen/EditPresenter.hpp>
#include <gui/containers/OptionWheelConfig.hpp>

/**
 * @brief Screen for creating or editing an alarm.
 *
 * Steps through four sequential positions using R1 (next) / R2 (back):
 *   HOURS -> MINUTES -> REPEAT -> EFFECT
 *
 * Each position shows a different wheel; only the active wheel is visible.
 * Pressing R1 on the last step (EFFECT) saves the alarm.
 */
class EditView : public EditViewBase
{
public:
    EditView();
    virtual ~EditView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /** @brief Pre-fill all wheels with the given alarm values. */
    void set(uint8_t h, uint8_t m, Alarm::Repeat repeat, Alarm::Effect effect);

protected:
    /** @brief Editing step; determines which wheel is visible. */
    enum Position { HOURS = 0, MINUTES, REPEAT, EFFECT };
    Position mPosition{};

    touchgfx::Callback<EditView, OptionWheelItem&,       int16_t> mRepeatItemCb;
    touchgfx::Callback<EditView, OptionWheelCenterItem&, int16_t> mRepeatCenterItemCb;
    touchgfx::Callback<EditView, OptionWheelItem&,       int16_t> mEffectItemCb;
    touchgfx::Callback<EditView, OptionWheelCenterItem&, int16_t> mEffectCenterItemCb;

    void updateRepeatItem(OptionWheelItem& item, int16_t index);
    void updateRepeatCenterItem(OptionWheelCenterItem& item, int16_t index);
    void updateEffectItem(OptionWheelItem& item, int16_t index);
    void updateEffectCenterItem(OptionWheelCenterItem& item, int16_t index);

    /** @brief Switch the visible wheel and header label to the given @p id. */
    void setPosition(Position id);

    /** @brief Update the step-name label (e.g. "HOURS", "MIN.", "REPEAT", "ALERT"). */
    void setActiveName(TypedTextId msgId);

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // EDITVIEW_HPP
