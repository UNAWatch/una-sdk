#ifndef TOGGLE_HPP
#define TOGGLE_HPP

#include <gui_generated/containers/ToggleBase.hpp>

class Toggle : public ToggleBase
{
public:
    Toggle();
    virtual ~Toggle() {}

    virtual void initialize();

    void setState(bool state);
    bool getState() const;

    void setBackgroundGray(bool setGray);

protected:

    bool mState = false;
    bool mIsGray = false;
};

#endif // TOGGLE_HPP
