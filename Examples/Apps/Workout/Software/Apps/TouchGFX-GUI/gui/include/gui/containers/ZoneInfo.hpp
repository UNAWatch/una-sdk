#ifndef ZONEINFO_HPP
#define ZONEINFO_HPP

#include <ctime>

#include <gui_generated/containers/ZoneInfoBase.hpp>

/**
 * @brief One row of the HR-zones summary face: zone digit, percent text, time text,
 *        and a coloured progress bar.
 *
 * Layout and styling are authored in TouchGFX Designer (@ref ZoneInfoBase).  At
 * runtime each row is populated by @ref setRow with the zone number, the time
 * spent in that zone, the percent of the active session that represents, and
 * the colour to paint the progress bar.
 */
class ZoneInfo : public ZoneInfoBase
{
public:
    ZoneInfo();
    virtual ~ZoneInfo() {}

    virtual void initialize();

    /**
     * @brief Populate this row.
     * @param zoneNumber1to5 Zone digit shown on the left (1..5).
     * @param seconds        Seconds spent in this zone (formatted M:SS / MM:SS / H:MM:SS).
     * @param percent0to100  Share of total active session time, used for the bar
     *                       length and the percent text.  Clamped to [0,100].
     * @param barColor       Colour applied to the bar via its PainterABGR2222.
     */
    void setRow(uint8_t zoneNumber1to5,
                std::time_t seconds,
                float percent0to100,
                touchgfx::colortype barColor);

protected:
    /// Bar start x in the bar widget's local coordinate space (from generated base).
    static constexpr int16_t kBarStartX = 6;
    /// Bar end x at 100% (from generated base background line).
    static constexpr int16_t kBarFullEndX = 48;
    /// Bar y coordinate (from generated base).
    static constexpr int16_t kBarY = 10;
    /// Right edge (local x) the time text is right-aligned to.  Just inside the
    /// 180-wide container so a wide H:MM:SS grows leftward without being clipped
    /// by the container bounds; still clears the progress bar (ends near x=120).
    static constexpr int16_t kZoneTimeRightEdgeX = 179;
};

#endif // ZONEINFO_HPP
