#ifndef SUMMARYFACEZONES_HPP
#define SUMMARYFACEZONES_HPP

#include <gui_generated/containers/SummaryFaceZonesBase.hpp>

#include "ActivitySummary.hpp"

/**
 * @brief Summary face showing time spent in each heart-rate zone.
 *
 * Five rows are laid out by TouchGFX Designer with zone 5 at the top and zone 1
 * at the bottom (see @ref SummaryFaceZonesBase).  Per-zone seconds come from
 * @ref ActivitySummary::zoneTimeSec; any active seconds the session spent below
 * zone 1 are absorbed into the zone-1 row for display only.  Percentages are
 * computed against @ref ActivitySummary::time so all five rows sum to 100%.
 */
class SummaryFaceZones : public SummaryFaceZonesBase
{
public:
    SummaryFaceZones();
    virtual ~SummaryFaceZones() {}

    virtual void initialize();

    /** @brief Populate all five rows from the activity summary. */
    void setZoneSummary(const ActivitySummary& s);

protected:
};

#endif // SUMMARYFACEZONES_HPP
