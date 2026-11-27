// Shared helpers for the Session tests: road routes to and from a
// track, road shapes that must not look like a track, a stored track with a
// day's GPS offset, and checks on a Session replay.
#pragma once

#include "OvalTestUtil.hpp"
#include "Replay.hpp"
#include "TraceGen.hpp"

#include "Oval/Session.hpp"
#include "Oval/TrackStore.hpp"

#include <cmath>
#include <random>
#include <vector>

namespace SessionTest {

using TraceGen::Pt;

constexpr double kPi = 3.141592653589793;

inline double distOf(Pt a, Pt b)
{
    return PortableMath::hypot(a.u - b.u, a.v - b.v);
}

inline double norm(Pt a)
{
    return PortableMath::hypot(a.u, a.v);
}

// Road legs going away from the track, from `from`, about lengthM long:
// corners every 150 to 450 m, never back within 250 m of the track's centre
// once past it.
inline std::vector<Pt> legsAway(Pt from, double headingRad, double lengthM, std::mt19937& rng)
{
    PortableRandom::Uniform leg(150.0, 450.0), turn(-1.2, 1.2), away(-0.8, 0.8);
    std::vector<Pt> pts;
    Pt     p = from;
    double h = headingRad, done = 0.0;
    while (done < lengthM) {
        const double l = std::min(leg(rng), lengthM - done + 1.0);
        Pt q{p.u + l * PortableMath::cos(h), p.v + l * PortableMath::sin(h)};
        if (norm(q) < 300.0 && !pts.empty()) {
            // Heading back towards the track: turn away from it instead.
            h = PortableMath::atan2(p.v, p.u) + away(rng);
            q = {p.u + l * PortableMath::cos(h), p.v + l * PortableMath::sin(h)};
        }
        pts.push_back(q);
        done += distOf(p, q);
        p = q;
        h += turn(rng);
    }
    return pts;
}

// The outward direction at a station, in track coordinates.
inline double outwardAt(const TraceGen::Generator& g, double tau, uint8_t lane)
{
    const Pt a = g.trackPointAt(tau, lane, 0.0);
    const Pt b = g.trackPointAt(tau, lane, 1.0);
    return PortableMath::atan2(b.v - a.v, b.u - a.u);
}

// A road from home (returned in `home`) to the track, lengthM long, coming
// onto lane's line at station tau from about 20 m outside, slightly before it.
inline std::vector<Pt> routeIn(const TraceGen::Generator& g, double tau, uint8_t lane, double lengthM,
                               std::mt19937& rng, Pt& home, double backM = 12.0, double outM = 22.0)
{
    // The last leg comes from backM before the entry along the lane and outM
    // outside it (12 m and 22 m by default).
    const Pt entry = g.trackPointAt(tau, lane, 0.0);
    const Pt gate  = g.trackPointAt(g.tauAfter(tau, lane, -backM), lane, outM);
    PortableRandom::Uniform wobble(-0.5, 0.5);
    std::vector<Pt> out = legsAway(gate, outwardAt(g, tau, lane) + wobble(rng), lengthM, rng);
    std::vector<Pt> route(out.rbegin(), out.rend());
    home = route.empty() ? gate : route.front();
    if (!route.empty()) {
        route.erase(route.begin());
    }
    route.push_back(gate);
    route.push_back(entry);
    return route;
}

// A road from where the runner is on the track, out across the lanes and
// away, lengthM long.
inline std::vector<Pt> routeOut(const TraceGen::Generator& g, uint8_t lane, double lengthM, std::mt19937& rng)
{
    const double tau = g.tauNow();
    const Pt     off = g.trackPointAt(tau, lane, 25.0);
    PortableRandom::Uniform wobble(-0.5, 0.5);
    std::vector<Pt> route{off};
    const std::vector<Pt> rest = legsAway(off, outwardAt(g, tau, lane) + wobble(rng), lengthM - 25.0, rng);
    route.insert(route.end(), rest.begin(), rest.end());
    return route;
}

// Closed road shapes, as polylines from (and back to) a point, `times` times.
inline std::vector<Pt> loopShape(const std::vector<Pt>& shape, Pt at, int times)
{
    std::vector<Pt> r;
    for (int k = 0; k < times; ++k) {
        for (const Pt& p : shape) {
            r.push_back({at.u + p.u, at.v + p.v});
        }
    }
    r.push_back({at.u + shape.front().u, at.v + shape.front().v});
    return r;
}

// A 400 m irregular park loop (TraceGen::parkLoopTrack), starting at its first point.
inline std::vector<Pt> parkLoop(double lapM = 400.0)
{
    const TraceGen::Track t = TraceGen::parkLoopTrack(lapM);
    std::vector<Pt> r;
    for (size_t i = 0; i < t.pathU.size(); i += 8) {
        r.push_back({t.pathU[i], t.pathV[i]});
    }
    return r;
}

// A circle (a roundabout), radiusM.
inline std::vector<Pt> circle(double radiusM)
{
    std::vector<Pt> r;
    for (int i = 0; i < 36; ++i) {
        const double a = -kPi / 2.0 + 2.0 * kPi * i / 36.0;
        r.push_back({radiusM * PortableMath::cos(a), radiusM + radiusM * PortableMath::sin(a)});
    }
    return r;
}

// A figure of eight: two lobes of radiusM either side of the start.
inline std::vector<Pt> figureEight(double radiusM)
{
    std::vector<Pt> r;
    for (int i = 0; i < 36; ++i) {
        const double a = 2.0 * kPi * i / 36.0;
        r.push_back({radiusM - radiusM * PortableMath::cos(a), radiusM * PortableMath::sin(a)});
    }
    for (int i = 0; i < 36; ++i) {
        const double a = 2.0 * kPi * i / 36.0;
        r.push_back({-radiusM + radiusM * PortableMath::cos(a), radiusM * PortableMath::sin(a)});
    }
    return r;
}

// A car park perimeter: a rectangle with square corners.
inline std::vector<Pt> carPark(double w, double h)
{
    return {{0.0, 0.0}, {w, 0.0}, {w, h}, {0.0, h}};
}

// The true track as stored on another day: today's GPS sits (eastM, northM)
// away from where it was then.
inline uint8_t storeTrack(Oval::TrackStore& store, const TraceGen::Track& tk, double eastM, double northM,
                          uint8_t lane = 0)
{
    const Oval::TrackModel truth  = Replay::truthModel(tk);
    const Oval::TrackModel stored = Replay::shiftedModel(truth, -eastM, -northM);
    Oval::FitQuality q;
    q.scaleM = 2.5f;
    q.shapeM = 0.2f;
    const Oval::TrackStore::SaveResult s = store.saveNew(stored, q, 1760000000u);
    if (lane != 0) {
        Oval::StoreOutcome o;
        o.kind = Oval::StoreOutcome::Kind::Unchanged;
        o.lane = lane;
        store.apply(o, s.number, 1760000001u);
    }
    return s.number;
}

// A track somewhere round Edinburgh, at a random heading and radius.
inline TraceGen::Track randomTrack(std::mt19937& rng)
{
    PortableRandom::Uniform u01(0.0, 1.0);
    TraceGen::Track tk;
    tk.centreLat   = 55.90 + 0.1 * u01(rng);
    tk.centreLon   = -3.30 + 0.2 * u01(rng);
    tk.headingDeg  = static_cast<float>(180.0 * u01(rng));
    tk.kerbRadiusM = static_cast<float>(35.0 + 3.0 * u01(rng));
    return tk;
}

inline TraceGen::Speed jog(float mps)
{
    TraceGen::Speed s = OvalTest::runner(mps);
    s.bendFactor = 1.0f;
    return s;
}

// Track laps (Lap and MissedLap) the Session counted in its track segment
// number k (0-based, counting Track segments that began with a join or a
// track start), and whether each was credited exactly the lane's length.
struct SegLaps {
    size_t laps  = 0;
    size_t exact = 0;
};

inline SegLaps lapsBetween(const Replay::SessionResult& r, uint32_t fromMs, uint32_t toMs, float laneLenM)
{
    SegLaps s;
    for (const Oval::Session::Event& e : r.laps) {
        if (Oval::msDiff(e.tMs, fromMs) >= 0 && (toMs == 0 || Oval::msDiff(toMs, e.tMs) >= 0)) {
            ++s.laps;
            if (std::fabs(e.track.lapLengthM - laneLenM) <= 0.01f) {
                ++s.exact;
            }
        }
    }
    return s;
}

// The reading at (or just after) a time.
inline size_t sampleAtOrAfter(const TraceGen::Trace& tr, uint32_t tMs)
{
    for (size_t i = tr.startIndex; i < tr.samples.size(); ++i) {
        if (Oval::msDiff(tr.samples[i].tMs, tMs) >= 0) {
            return i;
        }
    }
    return tr.samples.size() - 1;
}

// Laps the runner really completed from the reading at the Session's join
// time to the reading at its exit time (forward progress along the line,
// whole laps). ambiguous: within marginM of a whole lap, where either count
// is right.
inline size_t trueLapsBetween(const TraceGen::Trace& tr, size_t from, size_t to, double laneLenM,
                              double marginM, bool& ambiguous)
{
    const double d    = tr.truePhysM[to] - tr.truePhysM[from];
    const double laps = d / laneLenM;
    const double frac = laps - std::floor(laps);
    ambiguous = frac * laneLenM < marginM || (1.0 - frac) * laneLenM < marginM;
    return static_cast<size_t>(std::floor(laps));
}

// The GPS distance (kernel total) added at reading i.
inline double gpsStep(const TraceGen::Trace& tr, size_t i)
{
    if (i == 0 || !std::isfinite(tr.samples[i].gpsDistM) || !std::isfinite(tr.samples[i - 1].gpsDistM)) {
        return 0.0;
    }
    return static_cast<double>(tr.samples[i].gpsDistM) - static_cast<double>(tr.samples[i - 1].gpsDistM);
}

// Between the Session's transitions and the true ones: the GPS distance and
// the track distance run there, and their difference.
struct Allowance {
    double gpsM        = 0.0;
    double trackM      = 0.0;
    double diffM       = 0.0;
    size_t transitions = 0;
};

inline Allowance allowance(const TraceGen::Trace& tr, const Replay::SessionResult& r)
{
    Allowance a;
    auto between = [&](size_t x, size_t y) {
        const size_t lo = std::min(x, y), hi = std::max(x, y);
        double gps = 0.0;
        for (size_t i = lo + 1; i <= hi; ++i) {
            gps += gpsStep(tr, i);
        }
        const double trk = tr.truePhysM[hi] - tr.truePhysM[lo];
        a.gpsM   += gps;
        a.trackM += trk;
        a.diffM  += std::fabs(gps - trk);
        ++a.transitions;
    };
    for (size_t v = 0; v < tr.visits.size() && 2 + 2 * v < r.changes.size(); ++v) {
        between(sampleAtOrAfter(tr, r.changes[1 + 2 * v].tMs), tr.visits[v].joinSample);
        if (tr.visits[v].exitMs != 0) {
            between(sampleAtOrAfter(tr, r.changes[2 + 2 * v].tMs), tr.visits[v].exitSample);
        }
    }
    return a;
}

} // namespace SessionTest
