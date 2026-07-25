#include <gui/containers/OptionWheel.hpp>

OptionWheel::OptionWheel()
{
}

void OptionWheel::initialize()
{
    OptionWheelBase::initialize();
}

void OptionWheel::invalidate()
{
    for (int i = 0; i < wheel.getNumberOfItems(); i++) {
        wheel.itemChanged(i);
    }
    wheel.invalidate();
}

void OptionWheel::setNumberOfItems(int16_t numberOfItems)
{
    wheel.setNumberOfItems(numberOfItems);
}

int16_t OptionWheel::getNumberOfItems()
{
    return wheel.getNumberOfItems();
}

void OptionWheel::selectNext()
{
    if (wheel.getNumberOfItems() <= 1) {
        return;
    }
    int16_t p = static_cast<int16_t>(wheel.getSelectedItem()) + 1;
    wheel.animateToItem(p, kAnimationSteps);
}

void OptionWheel::selectPrev()
{
    if (wheel.getNumberOfItems() <= 1) {
        return;
    }
    int16_t p = static_cast<int16_t>(wheel.getSelectedItem()) - 1;
    wheel.animateToItem(p, kAnimationSteps);
}

void OptionWheel::selectItem(int16_t itemIndex)
{
    wheel.animateToItem(itemIndex, 0);
}

uint16_t OptionWheel::getSelectedItem()
{
    return wheel.getSelectedItem();
}

touchgfx::ScrollWheelWithSelectionStyle& OptionWheel::getWheel()
{
    return wheel;
}

void OptionWheel::setUpdateItemCallback(
    touchgfx::GenericCallback<OptionWheelItem&, int16_t>& cb)
{
    mpUpdateItemCb = &cb;
}

void OptionWheel::setUpdateCenterItemCallback(
    touchgfx::GenericCallback<OptionWheelCenterItem&, int16_t>& cb)
{
    mpUpdateCenterItemCb = &cb;
}

void OptionWheel::wheelUpdateItem(OptionWheelItem& item, int16_t itemIndex)
{
    if (mpUpdateItemCb != nullptr && mpUpdateItemCb->isValid()) {
        mpUpdateItemCb->execute(item, itemIndex);
    }
}

void OptionWheel::wheelUpdateCenterItem(OptionWheelCenterItem& item, int16_t itemIndex)
{
    if (mpUpdateCenterItemCb != nullptr && mpUpdateCenterItemCb->isValid()) {
        mpUpdateCenterItemCb->execute(item, itemIndex);
    }
}
