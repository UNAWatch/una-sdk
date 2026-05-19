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

void LapListItem::setTime(const touchgfx::Unicode::UnicodeChar* text)
{
    Unicode::strncpy(timeTextBuffer, text, TIMETEXT_SIZE);
    timeText.invalidate();
}

void LapListItem::setAvgHR(const touchgfx::Unicode::UnicodeChar* text)
{
    Unicode::strncpy(avgHRTextBuffer, text, AVGHRTEXT_SIZE);
    avgHRText.invalidate();
}

void LapListItem::setMaxHR(const touchgfx::Unicode::UnicodeChar* text)
{
    Unicode::strncpy(maxHRTextBuffer, text, MAXHRTEXT_SIZE);
    maxHRText.invalidate();
}
