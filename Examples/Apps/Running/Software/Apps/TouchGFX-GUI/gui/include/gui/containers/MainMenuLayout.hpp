#ifndef MAINMENULAYOUT_HPP
#define MAINMENULAYOUT_HPP

#include <gui_generated/containers/MainMenuLayoutBase.hpp>
#include <gui/containers/MenuItemConfig.hpp>
#include <touchgfx/Callback.hpp>

/**
 * @brief Standard full-screen menu layout container.
 *
 * Combines MainMenu (scroll wheel + scroll indicator) with the common chrome:
 * background image, title, infoText text, and navigation buttons.
 *
 * Most MainMenu operations are delegated directly, so screens can call
 * layout.setNumberOfItems(5) instead of layout.getMenu().setNumberOfItems(5).
 * For advanced wheel access use getMenu().
 *
 * Typical screen setup:
 * @code
 *   menuLayout.setTitle(T_TEXT_SETTINGS);
 *   menuLayout.setInfoMsg(T_TEXT_SIGNAL_ACQUIRED);
 *   menuLayout.getButtons().setR1(Buttons::AMBER);
 *   menuLayout.setUpdateItemCallback(mUpdateItemCb);
 *   menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
 *   menuLayout.setNumberOfItems(kNumItems);
 *   menuLayout.invalidate();
 * @endcode
 */
class MainMenuLayout : public MainMenuLayoutBase
{
public:
    MainMenuLayout();
    virtual ~MainMenuLayout() {}

    virtual void initialize();

    // ---- Delegated wheel operations ----------------------------------------
    void     setNumberOfItems(int16_t numberOfItems);
    int16_t  getNumberOfItems();
    void     selectNext();
    void     selectPrev();
    void     selectItem(int16_t itemIndex);
    uint16_t getSelectedItem();
    virtual void invalidate();

    void setUpdateItemCallback(
        touchgfx::GenericCallback<MainMenuItem &, int16_t> &cb);
    void setUpdateCenterItemCallback(
        touchgfx::GenericCallback<MainMenuCenterItem &, int16_t> &cb);

    /** @brief Apply geometry layout to all center items. */
    void setCenterItemLayout(const CenterItemLayout &layout);

    /** @brief Apply geometry layout to all surrounding items. */
    void setItemLayout(const ItemLayout &layout);

    void setAnimationSteps(int16_t steps);
    void setAnimationMiddleCallback(touchgfx::GenericCallback<int16_t> &cb);
    void setAnimationEndedCallback(touchgfx::GenericCallback<int16_t> &cb);

    // ---- Chrome ------------------------------------------------------------
    void setTitle(TypedTextId msgId);
    void showTitle(bool state);

    void setBackground(touchgfx::colortype color);
    void showBackground(bool state);

    void setInfoMsg(TypedTextId msgId);
    void setInfoMsgColor(touchgfx::colortype color);

    // ---- Getters -----------------------------------------------------------
    /** Direct access to navigation buttons (set L1/L2/R1/R2 state). */
    Buttons  &getButtons();
    /** Direct access to the scroll wheel menu (for advanced operations). */
    MainMenu &getMenu();
};

#endif // MAINMENULAYOUT_HPP
