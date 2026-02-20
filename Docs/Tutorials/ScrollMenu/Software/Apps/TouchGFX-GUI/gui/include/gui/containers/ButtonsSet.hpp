#ifndef BUTTONSSET_HPP
#define BUTTONSSET_HPP

#include <gui_generated/containers/ButtonsSetBase.hpp>

class ButtonsSet : public ButtonsSetBase
{
public:
    ButtonsSet();
    virtual ~ButtonsSet() {}

    virtual void initialize();

    enum Color {
        NONE = 0,
        WHITE,
        AMBER,
        RED,
    };

    void setL1(Color color = NONE);
    void setL2(Color color = NONE);
    void setR1(Color color = NONE);
    void setR2(Color color = NONE);

protected:
};

#endif // BUTTONSSET_HPP
