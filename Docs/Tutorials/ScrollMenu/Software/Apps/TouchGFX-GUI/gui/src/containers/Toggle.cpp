#include <gui/containers/Toggle.hpp>

Toggle::Toggle()
{

}

void Toggle::initialize()
{
    ToggleBase::initialize();
}

void Toggle::setState(bool state)
{
    mState = state;
    railOn.setVisible(mState);
    if (mIsGray) {
        railOff.setVisible(false);
        railOffGray.setVisible(!mState);
    }
    else {
        railOff.setVisible(!mState);
        railOffGray.setVisible(false);
    }

    if (mState) {
        handle.setX(30);
    }
    else {
        handle.setX(0);
    }
    invalidate();
}

bool Toggle::getState() const
{
    return mState;
}

void Toggle::setBackgroundGray(bool setGray)
{
    mIsGray = setGray;
    setState(mState);
}