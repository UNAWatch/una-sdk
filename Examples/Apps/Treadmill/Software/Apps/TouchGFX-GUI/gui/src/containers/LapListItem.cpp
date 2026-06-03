#include <gui/containers/LapListItem.hpp>

LapListItem::LapListItem()
{
}

void LapListItem::initialize()
{
    LapListItemBase::initialize();
}

void LapListItem::setIndex(const touchgfx::Unicode::UnicodeChar* text)
{
    Unicode::strncpy(indexTextBuffer, text, INDEXTEXT_SIZE);
    indexText.invalidate();
}

void LapListItem::setDistance(const touchgfx::Unicode::UnicodeChar* text)
{
    Unicode::strncpy(distanceTextBuffer, text, DISTANCETEXT_SIZE);
    distanceText.invalidate();
}

void LapListItem::setUnits(const touchgfx::Unicode::UnicodeChar* text)
{
    Unicode::strncpy(unitsTextBuffer, text, UNITSTEXT_SIZE);
    unitsText.invalidate();
}

void LapListItem::setTime(const touchgfx::Unicode::UnicodeChar* text)
{
    Unicode::strncpy(timeTextBuffer, text, TIMETEXT_SIZE);
    timeText.invalidate();
}

void LapListItem::setPace(const touchgfx::Unicode::UnicodeChar* text)
{
    Unicode::strncpy(paceTextBuffer, text, PACETEXT_SIZE);
    paceText.invalidate();
}
