#ifndef MAINMENU_HPP
#define MAINMENU_HPP

#include <gui_generated/containers/MainMenuBase.hpp>
#include <gui/containers/MenuItemConfig.hpp>
#include <touchgfx/Callback.hpp>

/**
 * @brief Scroll-wheel menu container (wheel + scroll only).
 *
 * Pure navigation component -- no chrome (title / buttons / info / background).
 * Use MainMenuLayout for the standard full-screen menu layout.
 *
 * Usage:
 *   1. Call setNumberOfItems() to configure the wheel and scroll counter.
 *   2. Register callbacks via setUpdateItemCallback() and
 *      setUpdateCenterItemCallback(). The screen owns MenuItemConfig arrays
 *      and calls item.apply(cfg) inside each callback.
 *   3. Optionally call setCenterItemLayout() / setItemLayout() to adjust
 *      item geometry for all items of each type.
 *   4. Navigate with selectNext() / selectPrev() / selectItem().
 *
 * Animation callbacks:
 *   setAnimationMiddleCallback() -- fires at the midpoint of the animation,
 *     useful for updating background before the new item is fully centered.
 *   setAnimationEndedCallback()  -- fires when animation completes.
 *   Both pass the target item index as argument.
 *
 * Layout priority (per item):
 *   MenuItemConfig::centerLayout / itemLayout
 *   >  MainMenu::setCenterItemLayout() / setItemLayout()
 *   >  built-in defaults
 */
class MainMenu : public MainMenuBase
{
public:
    MainMenu();
    virtual ~MainMenu() {}

    virtual void initialize();

    /** Re-render all wheel items (call after batch config changes). */
    virtual void invalidate();

    // ---- Wheel size --------------------------------------------------------
    void    setNumberOfItems(int16_t numberOfItems);
    int16_t getNumberOfItems();

    // ---- Navigation --------------------------------------------------------
    void     selectNext();
    void     selectPrev();
    void     selectItem(int16_t itemIndex);
    uint16_t getSelectedItem();

    // ---- Low-level access --------------------------------------------------
    touchgfx::ScrollWheelWithSelectionStyle &getWheel();

    // ---- Item callbacks ----------------------------------------------------
    /**
     * @brief Register the callback invoked for each surrounding item.
     * Signature: void callback(MainMenuItem &item, int16_t index)
     */
    void setUpdateItemCallback(
        touchgfx::GenericCallback<MainMenuItem &, int16_t> &cb);

    /**
     * @brief Register the callback invoked for the center item.
     * Signature: void callback(MainMenuCenterItem &item, int16_t index)
     */
    void setUpdateCenterItemCallback(
        touchgfx::GenericCallback<MainMenuCenterItem &, int16_t> &cb);

    // ---- Animation callbacks -----------------------------------------------
    /**
     * @brief Fires at the midpoint of a selectNext/selectPrev animation.
     *
     * Useful for updating the background while the old item is still
     * partially visible -- change happens smoothly mid-transition.
     * Not fired by selectItem() (instant, no animation).
     * Signature: void callback(int16_t targetItemIndex)
     */
    void setAnimationMiddleCallback(
        touchgfx::GenericCallback<int16_t> &cb);

    /**
     * @brief Fires when the wheel animation completes.
     *
     * Signature: void callback(int16_t selectedItemIndex)
     */
    void setAnimationEndedCallback(
        touchgfx::GenericCallback<int16_t> &cb);

    // ---- Animation speed ---------------------------------------------------
    /**
     * @brief Set the number of animation steps for selectNext/selectPrev.
     * Default: 4. Higher values = slower animation.
     */
    void setAnimationSteps(int16_t steps);

    // ---- Layout ------------------------------------------------------------
    /**
     * @brief Apply a geometry layout to all center items in the wheel buffer.
     * Individual items may still override it via MenuItemConfig::centerLayout.
     */
    void setCenterItemLayout(const CenterItemLayout &layout);

    /**
     * @brief Apply a geometry layout to all surrounding items in the wheel buffer.
     * Individual items may still override it via MenuItemConfig::itemLayout.
     */
    void setItemLayout(const ItemLayout &layout);

protected:
    virtual void wheelUpdateItem(MainMenuItem &item, int16_t itemIndex) override;
    virtual void wheelUpdateCenterItem(MainMenuCenterItem &item, int16_t itemIndex) override;
    virtual void handleTickEvent() override;

private:
    touchgfx::Callback<MainMenu> mWheelAnimationEndCb;
    void onWheelAnimationEnded();

    touchgfx::GenericCallback<MainMenuItem &,       int16_t> *mpUpdateItemCb       = nullptr;
    touchgfx::GenericCallback<MainMenuCenterItem &, int16_t> *mpUpdateCenterItemCb = nullptr;
    touchgfx::GenericCallback<int16_t>                       *mpAnimationMiddleCb  = nullptr;
    touchgfx::GenericCallback<int16_t>                       *mpAnimationEndedCb   = nullptr;

    int16_t mAnimSteps      = 4; ///< Steps per selectNext/selectPrev animation
    int16_t mAnimCountdown  = 0; ///< Ticks remaining in current animation
    int16_t mAnimTargetItem = 0; ///< Item index we are animating toward

    CenterItemLayout mCenterItemLayout;
    ItemLayout       mItemLayout;
};

#endif // MAINMENU_HPP
