/**
 ******************************************************************************
 * @file    TrackText.hpp
 * @brief   Track Run's words for where the run is: "Track 2", "New track",
 *          "Road", and the start screen's line.
 ******************************************************************************
 */

#ifndef TRACK_TEXT_HPP
#define TRACK_TEXT_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "Commands.hpp"
#include "Track.hpp"

namespace TrackText
{

/// "Track 2", "New track", "Road" or "Not a standard track" in a run; empty
/// before the first update.
inline void where(char* buf, size_t n, const Track::OnTrack& o)
{
    if (n == 0) {
        return;
    }
    if (o.segment == Track::OnTrack::ROAD) {
        std::snprintf(buf, n, "Road");
    } else if (o.segment != Track::OnTrack::TRACK) {
        buf[0] = '\0';
    } else if (o.cal == Track::OnTrack::NOT_STANDARD) {
        std::snprintf(buf, n, "Not a standard track");
    } else if (o.trackNumber != 0) {
        std::snprintf(buf, n, "Track %u", static_cast<unsigned>(o.trackNumber));
    } else {
        std::snprintf(buf, n, "New track");
    }
}

/// "Lane 3" or "No lane".
inline void lane(char* buf, size_t n, uint8_t lane)
{
    if (lane == 0) {
        std::snprintf(buf, n, "No lane");
    } else {
        std::snprintf(buf, n, "Lane %u", static_cast<unsigned>(lane));
    }
}

/// The start screen's line under Start: "Track 2, Lane 1", "New track,
/// Lane 1", or the lane alone before a fix. "No lane" starts on the road.
inline void preRun(char* buf, size_t n, uint8_t status, uint8_t trackNumber, uint8_t laneNumber)
{
    char l[12];
    lane(l, sizeof(l), laneNumber);
    if (laneNumber == 0 || status == CustomMessage::PreRunTrack::UNKNOWN) {
        std::snprintf(buf, n, "%s", l);
    } else if (status == CustomMessage::PreRunTrack::STORED) {
        std::snprintf(buf, n, "Track %u, %s", static_cast<unsigned>(trackNumber), l);
    } else {
        std::snprintf(buf, n, "New track, %s", l);
    }
}

/// "12 Oct" from UNIX seconds, shifted to local time by tzOffsetS; empty
/// for 0 (the date not known).
inline void shortDate(char* buf, size_t n, uint32_t unixS, int32_t tzOffsetS)
{
    if (n == 0) {
        return;
    }
    if (unixS == 0) {
        buf[0] = '\0';
        return;
    }
    static const char* const kMonths[12] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    const int64_t local = static_cast<int64_t>(unixS) + tzOffsetS;
    int64_t days = local >= 0 ? local / 86400 : (local - 86399) / 86400;
    // Civil from days (Howard Hinnant's algorithm), 1970-01-01 is day 0.
    days += 719468;
    const int64_t era = (days >= 0 ? days : days - 146096) / 146097;
    const int64_t doe = days - era * 146097;
    const int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const int64_t mp  = (5 * doy + 2) / 153;
    const int64_t d   = doy - (153 * mp + 2) / 5 + 1;
    const int64_t m   = mp < 10 ? mp + 3 : mp - 9;
    std::snprintf(buf, n, "%u %s", static_cast<unsigned>(d), kMonths[m - 1]);
}

} // namespace TrackText

#endif // TRACK_TEXT_HPP
