#ifndef OPTIONWHEEL_HPP
#define OPTIONWHEEL_HPP

#include <gui_generated/containers/OptionWheelBase.hpp>
#include <gui/containers/OptionWheelConfig.hpp>
#include <touchgfx/Callback.hpp>

/**
 * @brief Scroll-wheel container for selecting from a list of labeled options.
 *
 * The wheel is stateless: it holds no item data itself. The screen registers
 * two callbacks that are invoked whenever an item needs to be rendered:
 *   - updateItemCallback       -- for surrounding (non-centered) items
 *   - updateCenterItemCallback -- for the currently selected (center) item
 *
 * Typical setup in a screen's setupScreen():
 * @code
 *   menu.setUpdateItemCallback(mItemCb);
 *   menu.setUpdateCenterItemCallback(mCenterItemCb);
 *   menu.setNumberOfItems(Alarm::ACTION_COUNT);
 *   menu.invalidate();
 * @endcode
 */
class OptionWheel : public OptionWheelBase
{
public:
    OptionWheel();
    virtual ~OptionWheel() {}

    virtual void initialize();
    virtual void invalidate();

    /** @brief Set the total number of selectable items. Must be called before use. */
    void    setNumberOfItems(int16_t numberOfItems);
    int16_t getNumberOfItems();

    /** @brief Scroll to the next item (wraps around). */
    void selectNext();
    /** @brief Scroll to the previous item (wraps around). */
    void selectPrev();
    /** @brief Jump directly to the item at @p itemIndex. */
    void selectItem(int16_t itemIndex);
    /** @brief Return the index of the currently centered item. */
    uint16_t getSelectedItem();

    /** @brief Direct access to the underlying scroll widget (for advanced layout use). */
    touchgfx::ScrollWheelWithSelectionStyle& getWheel();

    /** @brief Register the callback invoked when a surrounding item needs to be rendered. */
    void setUpdateItemCallback(
        touchgfx::GenericCallback<OptionWheelItem&, int16_t>& cb);

    /** @brief Register the callback invoked when the center (selected) item needs to be rendered. */
    void setUpdateCenterItemCallback(
        touchgfx::GenericCallback<OptionWheelCenterItem&, int16_t>& cb);

protected:
    virtual void wheelUpdateItem(OptionWheelItem& item, int16_t itemIndex) override;
    virtual void wheelUpdateCenterItem(OptionWheelCenterItem& item, int16_t itemIndex) override;

private:
    /** @brief Number of ticks for the snap-scroll animation. */
    static constexpr int kAnimationSteps = 4;

    touchgfx::GenericCallback<OptionWheelItem&,       int16_t>* mpUpdateItemCb       = nullptr;
    touchgfx::GenericCallback<OptionWheelCenterItem&, int16_t>* mpUpdateCenterItemCb = nullptr;
};

#endif // OPTIONWHEEL_HPP
