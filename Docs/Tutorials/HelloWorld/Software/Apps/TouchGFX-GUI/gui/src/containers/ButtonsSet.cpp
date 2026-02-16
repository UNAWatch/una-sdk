#include <gui/containers/ButtonsSet.hpp>

ButtonsSet::ButtonsSet()
{

}

void ButtonsSet::initialize()
{
    ButtonsSetBase::initialize();
}


void ButtonsSet::setL1(Color color)
{
    buttonL1.setVisible(false);
    buttonL1a.setVisible(false);
    buttonL1r.setVisible(false);

    switch (color) {
    case WHITE:
        buttonL1.setVisible(true);
        break;
    case AMBER:
        buttonL1a.setVisible(true);
        break;
    case RED:
        buttonL1r.setVisible(true);
        break;
    default:
        break;
    }

    buttonL1.invalidate();
    buttonL1a.invalidate();
    buttonL1r.invalidate();
}

void ButtonsSet::setL2(Color color)
{
    buttonL2.setVisible(false);
    buttonL2a.setVisible(false);
    buttonL2r.setVisible(false);

    switch (color) {
    case WHITE:
        buttonL2.setVisible(true);
        break;
    case AMBER:
        buttonL2a.setVisible(true);
        break;
    case RED:
        buttonL2r.setVisible(true);
        break;
    default:
        break;
    }

    buttonL2.invalidate();
    buttonL2a.invalidate();
    buttonL2r.invalidate();
}

void ButtonsSet::setR1(Color color)
{
    buttonR1.setVisible(false);
    buttonR1a.setVisible(false);
    buttonR1r.setVisible(false);

    switch (color) {
    case WHITE:
        buttonR1.setVisible(true);
        break;
    case AMBER:
        buttonR1a.setVisible(true);
        break;
    case RED:
        buttonR1r.setVisible(true);
        break;
    default:
        break;
    }

    buttonR1.invalidate();
    buttonR1a.invalidate();
    buttonR1r.invalidate();
}

void ButtonsSet::setR2(Color color)
{
    buttonR2.setVisible(false);
    buttonR2a.setVisible(false);
    buttonR2r.setVisible(false);

    switch (color) {
    case WHITE:
        buttonR2.setVisible(true);
        break;
    case AMBER:
        buttonR2a.setVisible(true);
        break;
    case RED:
        buttonR2r.setVisible(true);
        break;
    default:
        break;
    }

    buttonR2.invalidate();
    buttonR2a.invalidate();
    buttonR2r.invalidate();
}
