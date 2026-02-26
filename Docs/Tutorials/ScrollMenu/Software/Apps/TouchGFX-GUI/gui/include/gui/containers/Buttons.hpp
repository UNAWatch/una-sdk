#ifndef BUTTONS_HPP
#define BUTTONS_HPP

#include <gui_generated/containers/ButtonsBase.hpp>

class Buttons : public ButtonsBase
{
public:
    Buttons();
    virtual ~Buttons() {}

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

#endif // BUTTONS_HPP
