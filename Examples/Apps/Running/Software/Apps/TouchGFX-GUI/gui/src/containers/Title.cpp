#include <gui/containers/Title.hpp>

Title::Title()
{
}

void Title::initialize()
{
    TitleBase::initialize();
}

void Title::set(TypedTextId msgId)
{
    touchgfx::Unicode::snprintf(titleTextBuffer, TITLETEXT_SIZE, "%s",
        touchgfx::TypedText(msgId).getText());
    titleText.invalidate();
}

void Title::set(touchgfx::Unicode::UnicodeChar* messageText)
{
    if (messageText != nullptr) {
        touchgfx::Unicode::snprintf(titleTextBuffer, TITLETEXT_SIZE, "%s", messageText);
        titleText.invalidate();
    }
}

void Title::set(const char* messageText)
{
    if (messageText != nullptr) {
        touchgfx::Unicode::fromUTF8(
            reinterpret_cast<const uint8_t*>(messageText), titleTextBuffer, TITLETEXT_SIZE);
        titleText.invalidate();
    }
}

void Title::setColor(touchgfx::colortype color)
{
    linePainter.setColor(color);
    line.invalidate();
    titleText.setColor(color);
    titleText.invalidate();
}
