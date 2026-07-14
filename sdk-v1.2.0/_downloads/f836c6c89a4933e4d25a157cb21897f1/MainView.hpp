#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>
#include <touchgfx/widgets/Image.hpp>
#include <touchgfx/widgets/ScalableImage.hpp>
#include <images/BitmapDatabase.hpp>

class MainView : public MainViewBase
{
public:
    MainView();
    virtual ~MainView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

protected:
    virtual void handleKeyEvent(uint8_t key) override;
    virtual void handleTickEvent() override;

    touchgfx::Image guyImage;
    touchgfx::ScalableImage scaledGuyImage;
    int16_t originalY;
    bool useScaledMode;
    bool isJumping;
    int animationTicks;
};

#endif // MAINVIEW_HPP
