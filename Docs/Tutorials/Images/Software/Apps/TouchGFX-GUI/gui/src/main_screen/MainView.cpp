#include <gui/main_screen/MainView.hpp>
#include <cmath>

MainView::MainView()
{
    useScaledMode = true;
    isJumping = false;
    animationTicks = 0;
    originalY = 100;
}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    buttons.setL1(ButtonsSet::AMBER);
    buttons.setR1(ButtonsSet::AMBER);
    buttons.setR2(ButtonsSet::WHITE);

    // Removed background usage - no background image or box used

    guyImage.setBitmap(touchgfx::Bitmap(BITMAP_GUY_TRANSPARENT_ID));
    guyImage.setPosition(70, originalY, 100, 100);
    guyImage.setVisible(false);
    add(guyImage);

    scaledGuyImage.setBitmap(touchgfx::Bitmap(BITMAP_GUY_TRANSPARENT_ID));
    scaledGuyImage.setPosition(70, originalY, 120, 120);
    scaledGuyImage.setScalingAlgorithm(touchgfx::ScalableImage::BILINEAR_INTERPOLATION);
    scaledGuyImage.setVisible(true);
    add(scaledGuyImage);
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

void MainView::handleKeyEvent(uint8_t key)
{
    if (key == Gui::Config::Button::L1) {
        useScaledMode = !useScaledMode;
        guyImage.setVisible(!useScaledMode);
        scaledGuyImage.setVisible(useScaledMode);
        guyImage.invalidate();
        scaledGuyImage.invalidate();
    }

    if (key == Gui::Config::Button::L2) {

    }

    if (key == Gui::Config::Button::R1) {
        if (!useScaledMode) {
            isJumping = true;
            animationTicks = 0;
        }
    }

    if (key == Gui::Config::Button::R2) {
        presenter->exit();
    }
}

void MainView::handleTickEvent()
{
    MainViewBase::handleTickEvent();

    if (isJumping && !useScaledMode) {
        animationTicks++;
        float phase = animationTicks * 0.1f;
        int offset = static_cast<int>(sin(phase) * 30.0f);
        guyImage.setY(originalY + offset);
        guyImage.invalidate();
        if (animationTicks > 60) {
            isJumping = false;
        }
    }
}