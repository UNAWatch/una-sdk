#ifndef MENUVIEW_HPP
#define MENUVIEW_HPP

#include <gui_generated/menu_screen/MenuViewBase.hpp>
#include <gui/menu_screen/MenuPresenter.hpp>
#include <gui/containers/OptionWheelConfig.hpp>

/**
 * @brief Action menu for a saved alarm: toggle on/off, edit, or delete.
 *
 * The wheel shows three options (ACTION_TOGGLE / ACTION_EDIT / ACTION_DELETE).
 * The center item for TOGGLE renders as a toggle switch reflecting mAlarmOn.
 */
class MenuView : public MenuViewBase
{
public:
    MenuView();
    virtual ~MenuView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /** @brief Set the alarm index and its current on/off state for display. */
    void setAlarmState(size_t id, bool state);

protected:
    size_t mId      = 0;      ///< Index of the alarm being actioned
    bool   mAlarmOn = false;  ///< Current on/off state (mirrored from model for the toggle widget)

    touchgfx::Callback<MenuView, OptionWheelItem&,       int16_t> mUpdateItemCb;
    touchgfx::Callback<MenuView, OptionWheelCenterItem&, int16_t> mUpdateCenterItemCb;

    void updateItem(OptionWheelItem& item, int16_t index);
    void updateCenterItem(OptionWheelCenterItem& item, int16_t index);

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // MENUVIEW_HPP
