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

    /** @brief Pre-fill all wheels with the given alarm values (@p h is 0-23). */
    void set(uint8_t h, uint8_t m, Alarm::Repeat repeat, Alarm::Effect effect);

    /** @brief Select 12- or 24-hour editing. Call before set(). */
    void setTimeFormat(bool is12Hour) { mIs12Hour = is12Hour; }

protected:
    /**
     * @brief Editing step; determines which wheel is visible.
     *
     * AMPM is only visited in 12-hour mode; in 24-hour mode it is skipped by
     * the R1/R2 navigation.
     */
    enum Position { HOURS = 0, MINUTES, AMPM, REPEAT, EFFECT };
    Position mPosition{};

    /// 12-hour editing (hours wheel shows 1-12 plus a separate AM/PM step).
    bool mIs12Hour = false;

    /// AM/PM step wheel (hand-added; 12-hour mode only). Shares the (20,55) slot.
    OptionWheel mAmPmMenu;

    touchgfx::Callback<EditView, OptionWheelItem&,       int16_t> mRepeatItemCb;
    touchgfx::Callback<EditView, OptionWheelCenterItem&, int16_t> mRepeatCenterItemCb;
    touchgfx::Callback<EditView, OptionWheelItem&,       int16_t> mEffectItemCb;
    touchgfx::Callback<EditView, OptionWheelCenterItem&, int16_t> mEffectCenterItemCb;
    touchgfx::Callback<EditView, OptionWheelItem&,       int16_t> mAmPmItemCb;
    touchgfx::Callback<EditView, OptionWheelCenterItem&, int16_t> mAmPmCenterItemCb;

    void updateRepeatItem(OptionWheelItem& item, int16_t index);
    void updateRepeatCenterItem(OptionWheelCenterItem& item, int16_t index);
    void updateEffectItem(OptionWheelItem& item, int16_t index);
    void updateEffectCenterItem(OptionWheelCenterItem& item, int16_t index);
    void updateAmPmItem(OptionWheelItem& item, int16_t index);
    void updateAmPmCenterItem(OptionWheelCenterItem& item, int16_t index);

    /** @brief Switch the visible wheel and header label to the given @p id. */
    void setPosition(Position id);

    /** @brief Update the step-name label (e.g. "HOURS", "MIN.", "REPEAT", "ALERT"). */
    void setActiveName(TypedTextId msgId);

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // EDITVIEW_HPP
