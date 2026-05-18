#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>
#include <touchgfx/widgets/Widget.hpp>
#include <touchgfx/Bitmap.hpp>
#include <touchgfx/hal/HAL.hpp>

// Minimal reproduction for a Y-clipping defect in
// LCD::drawPartialBitmap, observed on the Linux simulator with the
// prebuilt libtouchgfx.a 4.26.1. The widget draws a 50x50 ABGR2222
// dynamic external bitmap at screen (60, 220) so the source rect
// extends 30 px past the bottom of the 240x240 dirty rect, exercising
// the renderer's clipping path.
//
// Expected: a single red strip about 20 rows tall near the bottom of
// the widget; the upper part shows the default HelloWorld content.
//
// On the defective build, a second red strip also appears near the
// top of the widget because the clipping path writes destination rows
// starting at dirtyRect.y instead of screenY. Two red strips means
// the bug reproduces on your platform; a single strip near the bottom
// means it does not.
class DrawPartialBitmapBugRepro : public touchgfx::Widget
{
public:
    void setBitmap(touchgfx::BitmapId id) { mId = id; invalidate(); }

    virtual void draw(const touchgfx::Rect& /*area*/) const override
    {
        if (mId == touchgfx::BITMAP_INVALID) {
            return;
        }
        touchgfx::Bitmap bmp(mId);
        const touchgfx::Rect src(0, 0,
            static_cast<int16_t>(bmp.getWidth()),
            static_cast<int16_t>(bmp.getHeight()));
        touchgfx::HAL::lcd().drawPartialBitmap(bmp, 60, 220, src, 255, true);
    }

    virtual touchgfx::Rect getSolidRect() const override { return touchgfx::Rect(); }

private:
    touchgfx::BitmapId mId = touchgfx::BITMAP_INVALID;
};

class MainView : public MainViewBase
{
public:
    MainView();
    virtual ~MainView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

protected:
    virtual void handleKeyEvent(uint8_t key) override;

private:
    DrawPartialBitmapBugRepro reproWidget;
};

#endif // MAINVIEW_HPP
