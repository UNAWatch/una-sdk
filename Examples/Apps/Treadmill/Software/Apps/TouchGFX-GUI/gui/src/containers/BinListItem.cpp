#include <gui/containers/BinListItem.hpp>

BinListItem::BinListItem()
{
}

void BinListItem::initialize()
{
    BinListItemBase::initialize();
}

void BinListItem::setLabel(const touchgfx::Unicode::UnicodeChar* text)
{
    Unicode::strncpy(binLabelBuffer, text, BINLABEL_SIZE);
    binLabelBuffer[BINLABEL_SIZE - 1] = 0;   // guarantee null-termination
    binLabel.invalidate();
}

void BinListItem::setData(const touchgfx::Unicode::UnicodeChar* text, touchgfx::colortype color)
{
    Unicode::strncpy(dataPctBuffer, text, DATAPCT_SIZE);
    dataPctBuffer[DATAPCT_SIZE - 1] = 0;     // guarantee null-termination
    dataPct.setColor(color);
    dataPct.invalidate();
}
