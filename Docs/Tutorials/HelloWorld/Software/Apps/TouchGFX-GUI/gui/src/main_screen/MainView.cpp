#include <gui/main_screen/MainView.hpp>

namespace
{
// ABGR2222: one byte per pixel, bits MSB->LSB are AABBGGRR. Opaque
// red is alpha=3, blue=0, green=0, red=3 -> 0b11000011 = 0xC3.
constexpr uint8_t kABGR_Red = 0xC3;
constexpr uint16_t kReproDim = 50;
uint8_t  sReproPattern[kReproDim * kReproDim];
uint16_t sReproPool[2048];
} // namespace

MainView::MainView()
{

}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    for (size_t i = 0; i < sizeof(sReproPattern); ++i) {
        sReproPattern[i] = kABGR_Red;
    }
    touchgfx::Bitmap::setCache(sReproPool, sizeof(sReproPool), 1);
    const touchgfx::BitmapId id = touchgfx::Bitmap::dynamicBitmapCreateExternal(
        kReproDim, kReproDim, sReproPattern, touchgfx::Bitmap::ABGR2222);

    reproWidget.setPosition(0, 0, 240, 240);
    reproWidget.setBitmap(id);
    add(reproWidget);

    buttons.setL1(ButtonsSet::NONE);
    buttons.setL2(ButtonsSet::NONE);
    buttons.setR1(ButtonsSet::NONE);
    buttons.setR2(ButtonsSet::WHITE);
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

void MainView::handleKeyEvent(uint8_t key)
{
    if (key == Gui::Config::Button::L1) {

    }

    if (key == Gui::Config::Button::L2) {

    }

    if (key == Gui::Config::Button::R1) {
   
    }

    if (key == Gui::Config::Button::R2) {
        presenter->exit();
    }
}