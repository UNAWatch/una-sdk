#ifndef BATTERY_45X14_HPP
#define BATTERY_45X14_HPP

#include <gui_generated/containers/Battery_45x14Base.hpp>

/**
 * @brief Four-segment battery level indicator, 45 x 14.
 *
 * The segments are drawn at fixed coordinates rather than laid out, so the
 * container only works at the size it is named for; a different size is a
 * different container.
 *
 * Each segment (s1-s4) represents a 25 % charge band (left to right).
 * A segment is painted in the active color when the charge level reaches
 * its lower bound; otherwise it uses the inactive (background) color.
 *
 * Special case: s1 is shown in red when the level is below 25 %.
 *
 *  Level     s1          s2      s3      s4
 *  0 %       off         off     off     off
 *  1-24 %    red         off     off     off
 *  25-49 %   teal        teal    off     off
 *  50-74 %   teal        teal    teal    off
 *  75-100 %  teal        teal    teal    teal
 */
class Battery_45x14 : public Battery_45x14Base
{
public:
    Battery_45x14();
    virtual ~Battery_45x14() {}

    virtual void initialize();

    /**
     * @brief Update the displayed charge level.
     * @param level Charge percentage (0-100).
     */
    void setLevel(uint8_t level);

private:
    /** Paint a two-line segment (s1-s3). */
    void setSegment(touchgfx::Box& box,
                    touchgfx::Line& line1, touchgfx::PainterABGR2222& p1,
                    touchgfx::Line& line2, touchgfx::PainterABGR2222& p2,
                    touchgfx::colortype color);

    /** Paint the three-line segment (s4, includes terminal nub). */
    void setSegment4(touchgfx::colortype color);
};

#endif // BATTERY_45X14_HPP
