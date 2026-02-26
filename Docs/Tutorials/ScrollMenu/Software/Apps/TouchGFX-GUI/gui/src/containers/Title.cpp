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
    Unicode::snprintf(textBuffer, TEXT_SIZE, "%s", touchgfx::TypedText(msgId).getText());
}

void Title::set(touchgfx::Unicode::UnicodeChar *msg)
{
    if (msg != nullptr) {
        Unicode::snprintf(textBuffer, TEXT_SIZE, "%s", msg);
    }
}