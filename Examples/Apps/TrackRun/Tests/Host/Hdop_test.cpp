// The GPS precision figure is HDOP, not metres. A run with no HDOP behaves
// as the golden file records; the generator's HDOP is realistic and sets the noise;
// spells of poor HDOP on a straight, on a bend and across the gate.
//
// The golden file data/hdop_unknown_golden.txt holds the scenarios below with
// every precision figure unknown, as the Oval code handled them before it
// read HDOP, so a run with no HDOP at all must still give the same results.
// Regenerate it only on purpose, with:
//
//   trackrun-host-tests.exe --gtest_also_run_disabled_tests --gtest_filter=*WriteUnknownHdopGolden*
#include "OvalTestUtil.hpp"

#include <cstdarg>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>

using namespace OvalTest;

namespace {

#define TRACKRUN_UNKNOWN_PRECISION(s) ((s).hdop = -1.0f)

std::string goldenPath()
{
    return std::string(TRACE_DATA_DIR) + "/hdop_unknown_golden.txt";
}

void clearPrecision(TraceGen::Trace& tr)
{
    for (auto& s : tr.samples) {
        TRACKRUN_UNKNOWN_PRECISION(s);
    }
}

// Noise with no HDOP, so the white noise is the same at every reading.
TraceGen::Noise metresNoise(float sigmaM)
{
    TraceGen::Noise n = noisy(sigmaM);
    n.hdopModel = TraceGen::HdopModel::Unknown;
    return n;
}

// From scratch: calibration, the mid-run switch, a pause and a declared lane
// change.
TraceGen::Trace scratchScenario()
{
    TraceGen::Track track;
    track.headingDeg = 30.0f;
    TraceGen::Generator g(track, metresNoise(3.0f), 4242u);
    g.begin(0.1f, 3);
    g.runLaps(5.0, runner(5.0f));
    g.pause();
    g.stand(20.0);
    g.resume();
    g.changeLane(5);
    g.runLaps(4.0, runner(4.5f));
    g.run(40.0, runner(4.5f));
    TraceGen::Trace tr = g.finish();
    clearPrecision(tr);
    return tr;
}

// A stored track with a day's offset (lining up, drift tracking), and a
// move of four lanes the runner sets.
TraceGen::Trace storedScenario(TraceGen::Track& track)
{
    track.headingDeg = 110.0f;
    TraceGen::Generator g(track, metresNoise(2.5f), 777u);
    g.begin(0.6f, 1);
    g.runLaps(3.0, runner(5.0f));
    g.changeLane(5, 1.5f, true); // the runner sets lane 5
    g.runLaps(4.0, runner(5.0f));
    g.run(40.0, runner(5.0f));
    TraceGen::Trace tr = g.finish();
    clearPrecision(tr);
    return tr;
}

void put(std::ostringstream& os, const char* fmt, ...)
{
    char    buf[256];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    os << buf;
}

void writeTracker(std::ostringstream& os, const char* name, const Replay::TrackerResult& r)
{
    for (size_t k = 0; k < r.events.size(); ++k) {
        const auto& e = r.events[k];
        put(os, "%s ev %zu %d %u %u %u %.9g %.9g %d\n", name, r.eventSample[k], static_cast<int>(e.type),
            e.lapNumber, e.tMs, e.lapDurationMs, e.lapLengthM, e.totalM, static_cast<int>(e.mode));
    }
    for (size_t i = 0; i < r.total.size(); i += 3) {
        put(os, "%s tot %zu %.9g %d\n", name, i, r.total[i], r.mode[i]);
    }
    put(os, "%s end %.9g %d %d\n", name, r.finalTotal, static_cast<int>(r.finalMode), static_cast<int>(r.finalStage));
}

std::string goldenText()
{
    std::ostringstream os;

    const TraceGen::Trace a = scratchScenario();
    const Replay::Result c = Replay::run(a);
    for (size_t k = 0; k < c.events.size(); ++k) {
        const auto& e = c.events[k];
        put(os, "counter ev %zu %d %u %u %u %.9g %.9g\n", c.eventSample[k], static_cast<int>(e.type), e.lapNumber,
            e.tMs, e.lapDurationMs, e.lapLengthM, e.totalM);
    }
    for (size_t i = 0; i < c.total.size(); i += 3) {
        put(os, "counter tot %zu %.9g\n", i, c.total[i]);
    }
    writeTracker(os, "scratch", Replay::runTracker(a));

    TraceGen::Track track;
    const TraceGen::Trace b = storedScenario(track);
    const Oval::TrackModel stored = Replay::shiftedModel(Replay::truthModel(track), 6.0, -4.0);
    const Replay::TrackerResult rb = Replay::runTracker(b, Oval::LaneTable::outdoor(), Oval::Config(), &stored);
    writeTracker(os, "stored", rb);
    return os.str();
}

std::vector<std::string> fields(const std::string& line)
{
    std::istringstream is(line);
    std::vector<std::string> out;
    std::string f;
    while (is >> f) {
        out.push_back(f);
    }
    return out;
}

// One golden line against the same line now, field by field: the
// counts, indices, event types, lap numbers, modes and lanes exactly; metres
// within kGoldenM, times within kGoldenMs. The Oval code's own float maths
// differs in the last digit from one maths library to another (glibc against
// Windows' UCRT: 678.785522 m against 678.785461 m), so a bit-exact file
// would hold only where it was written; any change in behaviour still shows.
constexpr double kGoldenM  = 0.001;
constexpr double kGoldenMs = 1.0;

bool sameLine(const std::string& want, const std::string& got, std::string& why)
{
    const std::vector<std::string> a = fields(want), b = fields(got);
    if (a.size() != b.size() || a.size() < 2 || a[0] != b[0] || a[1] != b[1]) {
        why = "a different line";
        return false;
    }
    // What each field after the first two is: e exact, m metres, t milliseconds.
    const bool counter = a[0] == "counter";
    const char* kinds = a[1] == "ev"  ? (counter ? "eeettmm" : "eeettmme")
                      : a[1] == "tot" ? (counter ? "em" : "eme")
                      : a[1] == "end" ? "mee"
                                      : "";
    if (std::string(kinds).size() != a.size() - 2) {
        why = "an unknown line";
        return false;
    }
    for (size_t i = 2; i < a.size(); ++i) {
        const char k = kinds[i - 2];
        if (k == 'e') {
            if (a[i] != b[i]) {
                why = "field " + std::to_string(i) + " differs";
                return false;
            }
            continue;
        }
        // Both numbers to the end of the field, and finite: "nan" would pass
        // any tolerance.
        char*        endA = nullptr;
        char*        endB = nullptr;
        const double x    = std::strtod(a[i].c_str(), &endA);
        const double y    = std::strtod(b[i].c_str(), &endB);
        if (*endA != '\0' || *endB != '\0' || !std::isfinite(x) || !std::isfinite(y)) {
            why = "field " + std::to_string(i) + " is not a number";
            return false;
        }
        const double d = std::fabs(x - y);
        if (!(d <= (k == 'm' ? kGoldenM : kGoldenMs))) {
            why = "field " + std::to_string(i) + " is " + std::to_string(d) + (k == 'm' ? " m" : " ms") + " out";
            return false;
        }
    }
    return true;
}

} // namespace

TEST(Hdop, DISABLED_WriteUnknownHdopGolden)
{
    // The golden file's inputs are the default seed set's.
    ASSERT_EQ(PortableRandom::seedSet(), 0u) << "unset TRACKRUN_SEED_SET to write the golden file";
    // Written beside it and put in its place only when whole, so a failed
    // write leaves the old one.
    const std::string path = goldenPath();
    const std::string tmp  = path + ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(f.good()) << tmp;
        f << goldenText();
        f.close();
        ASSERT_FALSE(f.fail()) << "writing " << tmp;
    }
    std::remove(path.c_str());
    ASSERT_EQ(std::rename(tmp.c_str(), path.c_str()), 0) << tmp << " to " << path;
}

// A run with no HDOP at all behaves exactly as the golden file records: every
// event, and the total and mode at every third reading, to a millimetre and
// a millisecond (not to the last bit; see sameLine()).
TEST(Hdop, NoHdopAtAllMatchesGolden)
{
    if (PortableRandom::seedSet() != 0u) {
        GTEST_SKIP() << "the golden file's inputs are fixed: not run on another seed set";
    }
    std::ifstream f(goldenPath(), std::ios::binary);
    ASSERT_TRUE(f.good()) << goldenPath();
    std::stringstream want;
    want << f.rdbuf();
    std::istringstream a(want.str()), b(goldenText());
    std::string la, lb;
    size_t line = 0, diffs = 0, inexact = 0;
    std::string why;
    while (std::getline(a, la)) {
        ++line;
        if (!la.empty() && la.back() == '\r') {
            la.pop_back(); // a checkout may have given the file CRLF line endings
        }
        if (!std::getline(b, lb)) {
            ADD_FAILURE() << "the run is shorter than the golden file at line " << line;
            return;
        }
        inexact += la != lb ? 1 : 0;
        if (!sameLine(la, lb, why) && ++diffs <= 10) {
            ADD_FAILURE() << "line " << line << " (" << why << "): golden '" << la << "', now '" << lb << "'";
        }
    }
    std::printf("[ golden   ] %zu lines: %zu not bit for bit, %zu outside 1 mm and 1 ms\n", line, inexact, diffs);
    EXPECT_FALSE(std::getline(b, lb)) << "the run is longer than the golden file";
    EXPECT_EQ(diffs, 0u);
    EXPECT_GT(line, 700u);
}

// ---------------------------------------------------------------------------
// The generator's HDOP
// ---------------------------------------------------------------------------

// Realistic HDOP: 0.7 to 1.5, changing slowly, with a root mean square of
// 1.0 so the white noise keeps each test's noise band; random spells of 2 to
// 5 when asked for.
TEST(Hdop, GeneratorHdopIsRealistic)
{
    TraceGen::Track track;
    double sumSq = 0.0, maxStep = 0.0, sumStep = 0.0;
    size_t n = 0, inRange = 0;
    for (uint32_t seed = 1; seed <= 20; ++seed) {
        TraceGen::Generator g(track, noisy(3.0f), seed);
        g.begin(0.0f, 1);
        g.runLaps(10.0, runner(5.0f));
        const TraceGen::Trace tr = g.finish();
        for (size_t i = 0; i < tr.samples.size(); ++i) {
            const float h = tr.samples[i].hdop;
            sumSq += static_cast<double>(h) * h;
            ++n;
            inRange += (h >= 0.7f && h <= 1.5f) ? 1 : 0;
            if (i > 0) {
                const double step = std::fabs(h - tr.samples[i - 1].hdop);
                maxStep = std::max(maxStep, step);
                sumStep += step;
            }
        }
    }
    const double rms = std::sqrt(sumSq / n);
    std::printf("[ hdop     ] realistic: %zu readings, root mean square %.3f, %zu outside 0.7 to 1.5; change "
                "a second %.3f on average, at most %.2f\n", n, rms, n - inRange, sumStep / n, maxStep);
    EXPECT_NEAR(rms, 1.0, 0.03);
    EXPECT_EQ(inRange, n);
    // Slow: given to 0.1, it changes by a step about one second in five, and
    // never by more than two steps at once.
    EXPECT_LT(sumStep / n, 0.03);
    EXPECT_LE(maxStep, 0.2 + 1e-5);

    // Random spells.
    TraceGen::Noise sp = noisy(3.0f);
    sp.hdopSpellsPerHour = 6.0f;
    double spellSq = 0.0;
    size_t spellN = 0, spells = 0;
    for (uint32_t seed = 1; seed <= 20; ++seed) {
        TraceGen::Generator g(track, sp, seed);
        g.begin(0.0f, 1);
        g.runLaps(10.0, runner(5.0f));
        const TraceGen::Trace tr = g.finish();
        bool in = false;
        for (size_t i = 0; i < tr.samples.size(); ++i) {
            const float h = tr.samples[i].hdop;
            EXPECT_TRUE(h >= 0.7f && h <= 5.0f) << h;
            const bool spell = h >= 2.0f;
            spells += (spell && !in) ? 1 : 0;
            in = spell;
            if (spell) {
                spellSq += static_cast<double>(h) * h;
                ++spellN;
            }
        }
    }
    std::printf("[ hdop     ] random spells at 6 an hour: %zu in 20 runs of 10 laps, %zu readings, root mean "
                "square HDOP %.2f\n", spells, spellN, std::sqrt(spellSq / spellN));
    EXPECT_GT(spells, 5u);
    EXPECT_GT(std::sqrt(spellSq / spellN), 2.5);
}

// A reading's white noise is the noise at HDOP 1 times its HDOP, so poor
// spells really are noisier.
TEST(Hdop, GeneratorNoiseFollowsTheHdop)
{
    // Standing still with no drift: every reading's error is its white noise.
    TraceGen::Track track;
    double sq[2] = {0.0, 0.0}, hsq[2] = {0.0, 0.0};
    size_t cnt[2] = {0, 0};
    for (uint32_t seed = 1; seed <= 10; ++seed) {
        TraceGen::Generator g(track, noisy(3.0f, 0.0f), seed);
        g.begin(0.0f, 1);
        g.stand(120.0);
        g.poorHdop(true, 3.0f, 5.0f);
        g.stand(120.0);
        g.poorHdop(false);
        const TraceGen::Trace tr = g.finish();
        Oval::LocalFrame f;
        f.setOrigin(tr.startLat, tr.startLon);
        for (size_t i = tr.startIndex; i < tr.samples.size(); ++i) {
            const Oval::Vec2 p = f.toLocal(tr.samples[i].lat, tr.samples[i].lon);
            const int k = tr.samples[i].hdop >= 3.0f ? 1 : 0;
            sq[k] += 0.5 * (static_cast<double>(p.x) * p.x + static_cast<double>(p.y) * p.y);
            hsq[k] += static_cast<double>(tr.samples[i].hdop) * tr.samples[i].hdop;
            ++cnt[k];
        }
    }
    for (int k = 0; k < 2; ++k) {
        const double perAxis = std::sqrt(sq[k] / cnt[k]);
        const double hdopRms = std::sqrt(hsq[k] / cnt[k]);
        std::printf("[ hdop     ] %s: HDOP %.2f, white noise %.2f m an axis (3 m x HDOP: %.2f m)\n",
                    k ? "poor spell" : "normal", hdopRms, perAxis, 3.0 * hdopRms);
        EXPECT_NEAR(perAxis / (3.0 * hdopRms), 1.0, 0.1);
    }
}

// ---------------------------------------------------------------------------
// Poor-HDOP spells
// ---------------------------------------------------------------------------

namespace {

enum class Place { Straight, Bend, Gate };

const char* placeName(Place p)
{
    return p == Place::Straight ? "straight" : (p == Place::Bend ? "bend" : "gate");
}

struct SpellRun {
    TraceGen::Track track;
    TraceGen::Trace trace;
    size_t          spellFrom = 0; // readings in the spell
    size_t          spellTo   = 0;
};

// Seven laps from Start 20 m into the home straight, with a spell of HDOP 3
// to 5 lasting durS centred on the back straight, the second bend or the gate
// (the end of the lap), in lap spellLap.
SpellRun spellRun(Place place, double durS, int spellLap, uint8_t lane, float sigma, uint32_t seed)
{
    SpellRun out;
    TraceGen::Track& track = out.track;
    track.headingDeg  = 17.0f * static_cast<float>(seed % 20);
    track.kerbRadiusM = 35.5f + 0.25f * static_cast<float>(seed % 9);
    TraceGen::Generator g(track, noisy(sigma), seed);
    const double L = g.laneLengthM(lane);
    // The straights are the same in every lane; the bends take the rest.
    const double S = (400.0 - 2.0 * 3.141592653589793 * (track.kerbRadiusM + 0.30)) / 2.0;
    const double B = (L - 2.0 * S) / 2.0;
    const double startArc = 20.0;
    double toCentre = 0.0;
    switch (place) {
    case Place::Straight: toCentre = S + B + 0.5 * S - startArc + (spellLap - 1) * L; break;
    case Place::Bend:     toCentre = 2.0 * S + B + 0.5 * B - startArc + (spellLap - 1) * L; break;
    case Place::Gate:     toCentre = spellLap * L; break;
    }
    const float  mps  = 5.0f;
    const double half = 0.5 * durS * mps;

    g.begin(static_cast<float>(startArc / L), lane);
    g.run(toCentre - half, runner(mps));
    out.spellFrom = g.finish().samples.size(); // finish() copies; the run carries on
    g.poorHdop(true, 3.0f, 5.0f);
    g.run(2.0 * half, runner(mps));
    g.poorHdop(false);
    out.spellTo = g.finish().samples.size();
    g.run(7.0 * L - toCentre - half + 40.0, runner(mps));
    out.trace = g.finish();
    return out;
}


// Every lap counted and credited its lane's length, and timed: within
// realTolMs when it ended on a real crossing, within gapTolMs when it was
// credited across a gap in the readings (a spell of HDOP over maxHdop drops
// most of its readings), interpolated across it.
::testing::AssertionResult trackerLapsRight(const TraceGen::Trace& tr, const Replay::TrackerResult& r,
                                            float lapM, uint32_t realTolMs, uint32_t gapTolMs)
{
    if (r.decreases != 0) {
        return ::testing::AssertionFailure() << "total went down " << r.decreases << " times";
    }
    if (r.laps.size() != tr.crossMs.size()) {
        return ::testing::AssertionFailure() << "counted " << r.laps.size() << " laps, truth " << tr.crossMs.size();
    }
    for (size_t k = 0; k < r.laps.size(); ++k) {
        const bool    real = r.laps[k].type == Oval::Tracker::EventType::Lap;
        const int32_t err  = Oval::msDiff(r.laps[k].tMs, tr.crossMs[k]);
        if (std::abs(err) > static_cast<int32_t>(real ? realTolMs : gapTolMs)) {
            return ::testing::AssertionFailure() << "lap " << k + 1 << (real ? "" : " (across a gap)") << " ends "
                                                 << err << " ms from the truth";
        }
        if (!(std::fabs(r.laps[k].lapLengthM - lapM) <= 0.01f)) {   // a NaN length is wrong too
            return ::testing::AssertionFailure() << "lap " << k + 1 << " credited " << r.laps[k].lapLengthM << " m";
        }
    }
    return ::testing::AssertionSuccess();
}

} // namespace

// Spells of HDOP 3 to 5 for 20 to 60 s on a straight, on a bend and across
// the gate, in lap 2 (while calibrating) and lap 5 (calibrated), from scratch
// and on a stored track: no false or missed laps, in the Tracker and in
// counting mode alone, and calibration still locks.
TEST(Hdop, PoorSpellsOnAStraightABendAndAcrossTheGate)
{
    const Oval::LaneTable table = Oval::LaneTable::outdoor();
    size_t runs = 0, gapCredits = 0, counterGapCredits = 0, dropped = 0, spellReadings = 0;
    std::vector<double> realErr, gapErr, partLap, partLapSpell;
    for (Place place : {Place::Straight, Place::Bend, Place::Gate}) {
        for (double durS : {20.0, 40.0, 60.0}) {
            for (int spellLap : {2, 5}) {
                for (uint32_t seed = 1; seed <= 6; ++seed) {
                    const uint8_t lane  = static_cast<uint8_t>(1 + (seed * 3) % 8);
                    const float   sigma = 2.0f + 0.5f * static_cast<float>(seed % 5); // 2 to 4 m at HDOP 1
                    const SpellRun s = spellRun(place, durS, spellLap, lane, sigma, 9000u + seed);
                    const TraceGen::Trace& tr = s.trace;
                    for (size_t i = s.spellFrom; i < s.spellTo; ++i) {
                        dropped += tr.samples[i].hdop > Oval::Config().maxHdop ? 1 : 0;
                        ++spellReadings;
                    }
                    for (int stored = 0; stored <= 1; ++stored) {
                        char what[160];
                        std::snprintf(what, sizeof what, "%s, %d s in lap %d, %s, seed %u, lane %d, noise %.1f m",
                                      placeName(place), static_cast<int>(durS), spellLap,
                                      stored ? "stored track" : "from scratch", seed, lane, sigma);
                        const Oval::TrackModel model = Replay::truthModel(s.track);
                        const Replay::TrackerResult r =
                            Replay::runTracker(tr, table, Oval::Config(), stored ? &model : nullptr);
                        EXPECT_TRUE(trackerLapsRight(tr, r, table.lapLengthM(lane), 5000, 10000)) << what;
                        EXPECT_EQ(r.finalMode, Oval::Tracker::Mode::Calibrated) << what;
                        if (stored) {
                            EXPECT_EQ(r.finalStage, Oval::Tracker::CalStage::Supplied) << what;
                            EXPECT_TRUE(r.alignment.done && !r.alignment.abandoned) << what;
                        } else {
                            EXPECT_NE(r.lockedAt, SIZE_MAX) << "calibration did not lock: " << what;
                            EXPECT_EQ(r.finalStage, Oval::Tracker::CalStage::Locked) << what;
                        }
                        gapCredits += r.missed;
                        for (size_t k = 0; k < r.laps.size() && k < tr.crossMs.size(); ++k) {
                            const double e = std::fabs(Oval::msDiff(r.laps[k].tMs, tr.crossMs[k])) / 1000.0;
                            (r.laps[k].type == Oval::Tracker::EventType::Lap ? realErr : gapErr).push_back(e);
                        }
                        for (size_t i = tr.startIndex; i < tr.samples.size(); ++i) {
                            if (r.mode[i] == static_cast<uint8_t>(Oval::Tracker::Mode::Calibrated)) {
                                const bool inSpell = i >= s.spellFrom && i < s.spellTo + 10;
                                (inSpell ? partLapSpell : partLap).push_back(std::fabs(r.total[i] - tr.truePhysM[i]));
                            }
                        }
                        ++runs;
                    }

                    const Replay::Result c = Replay::run(tr, table);
                    EXPECT_EQ(c.decreases, 0u) << placeName(place) << " " << durS << " s, seed " << seed;
                    EXPECT_EQ(c.events.size(), tr.crossMs.size())
                        << "counting mode: " << placeName(place) << " " << durS << " s in lap " << spellLap
                        << ", seed " << seed;
                    counterGapCredits += c.missed;
                }
            }
        }
    }
    std::printf("[ spells   ] %zu Tracker runs (HDOP 3 to 5 for 20 to 60 s): %zu of %zu spell readings over "
                "HDOP 4 dropped; laps credited across a gap: %zu (Tracker), %zu (counting mode, %zu runs)\n",
                runs, dropped, spellReadings, gapCredits, counterGapCredits, runs / 2);
    printStats("lap end, real crossing (s)", realErr);
    printStats("lap end, across a gap (s)", gapErr);
    printStats("part-lap (m), away from spells", partLap);
    printStats("part-lap (m), in spells + 10 s", partLapSpell);
}
