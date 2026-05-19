#include <gui/containers/MainMenuBackground.hpp>

MainMenuBackground::MainMenuBackground()
{
}

void MainMenuBackground::initialize()
{
    MainMenuBackgroundBase::initialize();
}

void MainMenuBackground::setColor(touchgfx::colortype color)
{
    circlePainter.setColor(color);
    circle.invalidate();
}
