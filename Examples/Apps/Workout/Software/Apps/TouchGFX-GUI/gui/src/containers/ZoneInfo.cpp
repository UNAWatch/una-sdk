#include <gui/containers/ZoneInfo.hpp>

#include <algorithm>
#include <cmath>

#include <SDK/Utils/Utils.hpp>
#include <texts/TextKeysAndLanguages.hpp>

ZoneInfo::ZoneInfo()
{
}

void ZoneInfo::initialize()
{
    ZoneInfoBase::initialize();
}

void ZoneInfo::setRow(uint8_t zoneNumber1to5,
                      std::time_t seconds,
                      float percent0to100,
                      touchgfx::colortype barColor)
{
    Unicode::snprintf(zoneTextBuffer, ZONETEXT_SIZE, "%u",
                      static_cast<unsigned>(zoneNumber1to5));
    zoneText.invalidate();

    const auto hms = SDK::Utils::toHMS(seconds);
    if (hms.h > 0) {
        Unicode::snprintf(zoneTimeBuffer, ZONETIME_SIZE, "%u:%02u:%02u",
                          static_cast<unsigned>(hms.h),
                          static_cast<unsigned>(hms.m),
                          static_cast<unsigned>(hms.s));
    } else {
        Unicode::snprintf(zoneTimeBuffer, ZONETIME_SIZE, "%u:%02u",
                          static_cast<unsigned>(hms.m),
                          static_cast<unsigned>(hms.s));
    }
    zoneTime.invalidate();

    const float clampedPct = std::clamp(percent0to100, 0.0f, 100.0f);
    Unicode::snprintf(zonePercentBuffer, ZONEPERCENT_SIZE, "%u%s",
                      static_cast<unsigned>(clampedPct + 0.5f),
                      touchgfx::TypedText(T_TEXT_PERCENT).getText());
    zonePercent.invalidate();

    const bool barVisible = clampedPct > 0.0f;
    bar.setVisible(barVisible);
    if (barVisible) {
        const int16_t span = kBarFullEndX - kBarStartX;
        const int16_t endX = static_cast<int16_t>(
            kBarStartX + std::lround(span * clampedPct / 100.0f));
        bar.updateEnd(endX, kBarY);
    }
    barPainter.setColor(barColor);
    bar.invalidate();
}
