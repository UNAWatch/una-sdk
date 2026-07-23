#include <gui/containers/LapListItem.hpp>
#include <gui/common/TimeFormat.hpp>

LapListItem::LapListItem()
{

}

void LapListItem::initialize()
{
    LapListItemBase::initialize();
}

void LapListItem::setLap(uint8_t number, uint32_t durationMs)
{
    // The row reads "Lap <>": the word comes from the text database, only the
    // number is substituted here.
    touchgfx::Unicode::snprintf(indexTextBuffer, INDEXTEXT_SIZE, "%u", number);
    indexText.invalidate();

    TimeFormat::lapField(durationMs, timeTextBuffer, TIMETEXT_SIZE);
    timeText.invalidate();
}

void LapListItem::clear()
{
    indexTextBuffer[0] = 0;
    indexText.invalidate();

    timeTextBuffer[0] = 0;
    timeText.invalidate();
}
