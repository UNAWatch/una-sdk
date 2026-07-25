#include <gui/containers/MainMenu.hpp>
#include <touchgfx/Application.hpp>

MainMenu::MainMenu() :
    mWheelAnimationEndCb(this, &MainMenu::onWheelAnimationEnded)
{
}

void MainMenu::initialize()
{
    MainMenuBase::initialize();
    wheel.setAnimationEndedCallback(mWheelAnimationEndCb);
    touchgfx::Application::getInstance()->registerTimerWidget(this);
}

void MainMenu::invalidate()
{
    for (int i = 0; i < wheel.getNumberOfItems(); i++) {
        wheel.itemChanged(i);
    }
    wheel.invalidate();
}

void MainMenu::setNumberOfItems(int16_t numberOfItems)
{
    wheel.setNumberOfItems(numberOfItems);
    scrollIndicator.setCount(numberOfItems);
}

int16_t MainMenu::getNumberOfItems()
{
    return wheel.getNumberOfItems();
}

void MainMenu::selectNext()
{
    if (wheel.getNumberOfItems() <= 1) {
        return;
    }

    int16_t targetId = static_cast<int16_t>(wheel.getSelectedItem()) + 1;
    wheel.animateToItem(targetId, mAnimSteps);
    mAnimTargetItem = static_cast<int16_t>(wheel.getSelectedItem()); // actual index after wrap
    mAnimCountdown  = mAnimSteps;

    scrollIndicator.animateToId(targetId, mAnimSteps); // virtual id for correct wrap direction
}

void MainMenu::selectPrev()
{
    if (wheel.getNumberOfItems() <= 1) {
        return;
    }

    int16_t targetId = static_cast<int16_t>(wheel.getSelectedItem()) - 1;
    wheel.animateToItem(targetId, mAnimSteps);
    mAnimTargetItem = static_cast<int16_t>(wheel.getSelectedItem()); // actual index after wrap
    mAnimCountdown  = mAnimSteps;

    scrollIndicator.animateToId(targetId, mAnimSteps); // virtual id for correct wrap direction
}

void MainMenu::selectItem(int16_t itemIndex)
{
    if (wheel.getNumberOfItems() <= 1) {
        return;
    }

    // Instant jump -- no animation, no mid callback
    wheel.animateToItem(itemIndex, 0);
    scrollIndicator.setActiveId(wheel.getSelectedItem());
}

uint16_t MainMenu::getSelectedItem()
{
    return wheel.getSelectedItem();
}

touchgfx::ScrollWheelWithSelectionStyle &MainMenu::getWheel()
{
    return wheel;
}

void MainMenu::setUpdateItemCallback(
    touchgfx::GenericCallback<MainMenuItem &, int16_t> &cb)
{
    mpUpdateItemCb = &cb;
}

void MainMenu::setUpdateCenterItemCallback(
    touchgfx::GenericCallback<MainMenuCenterItem &, int16_t> &cb)
{
    mpUpdateCenterItemCb = &cb;
}

void MainMenu::setAnimationMiddleCallback(
    touchgfx::GenericCallback<int16_t> &cb)
{
    mpAnimationMiddleCb = &cb;
}

void MainMenu::setAnimationEndedCallback(
    touchgfx::GenericCallback<int16_t> &cb)
{
    mpAnimationEndedCb = &cb;
}

void MainMenu::setAnimationSteps(int16_t steps)
{
    mAnimSteps = steps;
}

void MainMenu::setCenterItemLayout(const CenterItemLayout &layout)
{
    mCenterItemLayout = layout;

    for (int i = 0; i < wheelSelectedListItems.getNumberOfDrawables(); i++) {
        wheelSelectedListItems[i].setLayout(mCenterItemLayout);
    }
}

void MainMenu::setItemLayout(const ItemLayout &layout)
{
    mItemLayout = layout;

    for (int i = 0; i < wheelListItems.getNumberOfDrawables(); i++) {
        wheelListItems[i].setLayout(mItemLayout);
    }
}

void MainMenu::wheelUpdateItem(MainMenuItem &item, int16_t itemIndex)
{
    if (mpUpdateItemCb != nullptr && mpUpdateItemCb->isValid()) {
        mpUpdateItemCb->execute(item, itemIndex);
    }
}

void MainMenu::wheelUpdateCenterItem(MainMenuCenterItem &item, int16_t itemIndex)
{
    if (mpUpdateCenterItemCb != nullptr && mpUpdateCenterItemCb->isValid()) {
        mpUpdateCenterItemCb->execute(item, itemIndex);
    }
}

void MainMenu::handleTickEvent()
{
    if (mAnimCountdown <= 0) {
        return;
    }

    mAnimCountdown--;

    // Fire middle callback at the halfway point of the animation
    if (mAnimCountdown == mAnimSteps / 2) {
        if (mpAnimationMiddleCb != nullptr && mpAnimationMiddleCb->isValid()) {
            mpAnimationMiddleCb->execute(mAnimTargetItem);
        }
    }
}

void MainMenu::onWheelAnimationEnded()
{
    mAnimCountdown = 0;

    if (mpAnimationEndedCb != nullptr && mpAnimationEndedCb->isValid()) {
        mpAnimationEndedCb->execute(static_cast<int16_t>(wheel.getSelectedItem()));
    }
}
