// Seeded synthetic GPS traces for the Oval host tests.
//
// A runner moves along the measurement line of a lane on a standard track
// (two straights and two semicircular bends), and the generator emits one
// reading a second, as the watch does. Positions carry Gaussian noise per axis
// plus a slow random-walk drift. The kernel's cumulative GPS distance is built
// by summing steps between noisy positions, as the simulator does.
// It has its own white jitter (distJitterM) and a
// scale, so its over-read can be set independently of the position noise:
// summing 3 to 5 m white noise directly would over-read by 40 to 100%, far
// more than the generator's distance scale is meant to give.
//
// Each reading reports an HDOP (the precision figure is HDOP, not
// metres). By default it wanders slowly over 0.7 to 1.5, and a reading's
// white noise is the noise figure times its HDOP, so spells of poor HDOP
// (random ones, or poorHdop() in a script) really are noisier. The HDOP has
// its own random stream, so the noise draws do not depend on the HDOP model.
//
// Every draw goes through PortableRandom, so the same seed gives
// the same trace on every compiler.
//
// The generator also keeps the truth: when the runner really passed the start
// point, the active time then, and what each lap should credit by the
// lane-change formula.
//
// Host only. Not part of the watch build.
#pragma once

#include "Oval/LocalFrame.hpp"
#include "Oval/Sample.hpp"
#include "PortableRandom.hpp"

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace TraceGen {

struct Track {
    double centreLat   = 55.95; // Edinburgh
    double centreLon   = -3.19;
    float  headingDeg  = 0.0f;  // home straight direction, counterclockwise from east
    float  kerbRadiusM = 36.5f;
    float  straightM   = 0.0f;  // 0: chosen so lane 1 is exactly 400 m

    // Non-standard shapes: when set, the runner follows this closed path
    // (track coordinates, anticlockwise, the last point joins the first)
    // whatever their lane, and a lap is the path's length.
    std::vector<double> pathU;
    std::vector<double> pathV;
};

// Non-standard shapes for the rejection tests, centred like a Track.
Track circleTrack(double lapM = 400.0);       // a 400 m circle
Track parkLoopTrack(double lapM = 400.0);     // an irregular loop of about 400 m
Track doubleBendTrack();                      // 400 m, compound bends of 34.0 and 51.543 m
Track shortTrack(double lane1M);              // standard shape, lane 1 this long (300 m)

// The HDOP each reading reports (the precision figure is HDOP).
enum class HdopModel : uint8_t {
    Realistic, // a slow wander over 0.7 to 1.5 (root mean square 1.0), with
               // occasional spells of 2 to 5
    Steady,    // hdopSteady throughout
    Unknown,   // no HDOP (-1), and the white noise is posSigmaM throughout
    MetresAccuracy, // the older accuracy figure (metresAccBaseM + |N(0, metresAccJitterM)|
               // metres, unrelated to the noise) as HDOP: divided by 4, as a version
               // 1 trace is read. The white noise is posSigmaM throughout.
};

struct Noise {
    // White noise per axis on reported positions, at HDOP 1. A reading's white
    // noise is this times its HDOP, so poor-HDOP spells really are noisier.
    // The realistic HDOP's root mean square over its normal range is 1.0, so
    // a run's overall noise is still about this.
    float    posSigmaM      = 0.0f;
    float    driftStepM     = 0.0f;  // random-walk drift, step per axis per reading
    float    distJitterM    = 0.0f;  // white noise per axis on the positions GPS distance sums
                                     // (not scaled by HDOP: it stands for the kernel's own
                                     // distance, matched to the simulator's over-read)
    float    distScale      = 1.0f;  // systematic scale on GPS distance
    HdopModel hdopModel     = HdopModel::Realistic;
    float    hdopSteady     = 1.0f;
    float    hdopSpellsPerHour = 0.0f;  // Realistic: random spells of poor HDOP (2 an
                                        // hour in the poor-HDOP sweep), each
    float    hdopSpellMinS  = 20.0f;    // lasting this long,
    float    hdopSpellMaxS  = 60.0f;
    float    hdopSpellLo    = 2.0f;     // at an HDOP wandering in this range
    float    hdopSpellHi    = 5.0f;
    float    metresAccBaseM  = 3.0f;  // the older accuracy figure, base + |N(0, jitter)| metres:
    float    metresAccJitterM = 1.5f; // drawn every reading to keep the stream, reported
                                     // only by HdopModel::MetresAccuracy
    float    badFixProb     = 0.0f;  // chance of a poor reading: HDOP 6 to 10, extra noise
    float    badFixSigmaM   = 15.0f;
    bool     floatCoords    = true;  // lat and lon rounded to float, as the sensor gives them
    bool     gpsDist        = true;  // false: gpsDistM is NaN throughout
    bool     dropoutDistance = false; // true: GPS distance keeps counting through a
                                      // dropout (dead reckoning); false: it stays
                                      // flat, then adds the chord, as the simulator does
    float    gpsDistStartM  = 1234.5f; // the kernel total is not zero at Start
    float    shiftEastM     = 0.0f;  // the whole GPS track moves by this much
    float    shiftNorthM    = 0.0f;  // after Start (not in the primed fixes)
    float    driftRateEMps  = 0.0f;  // steady drift after Start, east and north,
    float    driftRateNMps  = 0.0f;  // metres a second (on top of the random walk)
    uint32_t periodMs       = 1000;
    uint32_t periodJitterMs = 0;
};

struct Speed {
    float baseMps    = 5.0f;
    float variation  = 0.0f;  // amplitude of a slow sine, as a fraction of base
    float periodS    = 90.0f;
    float bendFactor = 1.0f;  // multiplier on the bends
    float jitter     = 0.0f;  // per-reading random fraction (standard deviation)
};

enum class CmdKind : uint8_t { Pause, Resume, SetLane, LapPress };

struct Command {
    size_t   beforeSample = 0; // apply before update() of this sample
    CmdKind  kind         = CmdKind::Pause;
    uint32_t tMs          = 0;
    uint8_t  lane         = 0;
};

// A point in track coordinates, metres: u along the straights, v across,
// from the track's centre (road routes are given this way, so they
// can lead onto the track).
struct Pt {
    double u = 0.0;
    double v = 0.0;
};

// One stay on the track, from joinTrack() to leaveToRoad().
struct TrackVisit {
    uint32_t joinMs   = 0;
    uint32_t exitMs   = 0;     // 0 while still on the track
    double   joinTau  = 0.0;   // station where the runner came onto the lane line
    size_t   firstLap = 0;     // index into crossMs of its first lap
    size_t   laps     = 0;     // laps completed in it
    double   partM    = 0.0;   // the part-lap run when leaving (truth formula)
    size_t   joinSample = 0;   // first reading on the track
    size_t   exitSample = 0;   // first reading off it
};

struct Trace {
    std::vector<Oval::Sample> samples;
    std::vector<float>        altM;
    std::vector<Command>      commands; // in sample order
    size_t   startIndex = 0;            // readings before this are pre-run (prime)
    uint32_t startMs    = 0;
    uint8_t  startLane  = 1;
    double   startLat   = 0.0;          // the true start point, without noise
    double   startLon   = 0.0;

    // Truth
    std::vector<uint32_t> crossMs;       // each lap's forward pass of the start point
    std::vector<uint32_t> crossActiveMs; // active time at each (pauses excluded)
    std::vector<double>   lapCreditM;    // what each lap should credit
    std::vector<double>   trueTrackM;    // per sample: credited laps + true part-lap
    std::vector<double>   truePhysM;     // per sample: track distance, the physical truth:
                                         // forward progress along the line actually run
                                         // (active, on the track), past the furthest point
                                         // reached, so going back and forth adds nothing
    std::vector<double>   lapPhysM;      // each lap's physical length on that basis
    std::vector<uint8_t>  pausedAt;      // per sample
    std::vector<float>    offsetE;       // per sample: the slow GPS error (drift and
    std::vector<float>    offsetN;       // shift, without white noise), east and north
    double movedM   = 0.0;               // metres moved along lane lines after Start
    double gpsDistM = 0.0;               // GPS distance added after Start

    // Road routes. Per sample: on the track (1) or the road (0),
    // the true road distance run so far (active), and the GPS distance
    // added while on the road. Laps after a join or a Lap press count from
    // there (crossMs carries on across visits).
    std::vector<uint8_t>    onTrack;
    std::vector<double>     trueRoadM;
    std::vector<double>     gpsRoadM;
    std::vector<TrackVisit> visits;
    std::vector<double>     trueTau;     // per sample: the runner's station (track coordinates), -1 on the road
};

class Generator {
public:
    Generator(const Track& track, const Noise& noise, uint32_t seed);

    // The script, in order. begin() emits primeReadings readings standing at
    // the start point, with Start half a second after the last of them.
    void begin(float startFrac, uint8_t lane, uint32_t t0Ms = 1000000u, int primeReadings = 6);
    // A rolling start. The runner runs in along the lane line for
    // runInM at sp (readings before Start are the primed fixes) and presses
    // Start as they pass the point startFrac of the lap, without slowing:
    // laps and the truth count from there.
    void beginRolling(float startFrac, uint8_t lane, const Speed& sp, double runInM, uint32_t t0Ms = 1000000u);
    void run(double metres, const Speed& sp);
    void runLaps(double laps, const Speed& sp);
    void stand(double seconds);
    void shuttle(double seconds, float amplitudeM, float speedMps, bool backwardFirst = false);
    void detour(float outM, float speedMps);
    // declare: the runner sets the new lane on the watch (a SetLane command
    // and the lane-change formula in the truth); false moves without telling it.
    void changeLane(uint8_t lane, float lateralMps = 1.5f, bool declare = true);
    // Leaves the track: out outM along the outward normal, on round the
    // outside by aheadM of track (not credited), and back in.
    void leaveTrack(float outM, float speedMps, double aheadM);
    void pause();
    void resume();

    // Road and track in one run. beginAt() starts on the road at a
    // point (prime readings standing there). road() runs along a polyline
    // (corners and all) from where the runner is, at a running speed, with
    // the same noise, drift, HDOP and GPS distance as on the track.
    // joinTrack() puts the runner on a lane line at a station (the road
    // should end there: trackPointAt()); laps then count from that station.
    // leaveToRoad() steps off it where the runner is. lapPress() is the Lap
    // button: laps count from here. setLane() is the runner setting the lane
    // here (0: "No lane"), with no move: a SetLane command only. beginAt()'s
    // lane is the lane given at Start (0: "No lane", a road start).
    void   beginAt(Pt start, uint8_t lane, uint32_t t0Ms = 1000000u, int primeReadings = 6);
    void   road(const std::vector<Pt>& route, const Speed& sp);
    void   joinTrack(double tau, uint8_t lane);
    void   leaveToRoad();
    void   lapPress();
    void   setLane(uint8_t lane);
    Pt     trackPointAt(double tau, uint8_t lane, double outwardM = 0.0) const;
    Pt     here() const;
    double tauAfter(double tau, uint8_t lane, double metres) const; // station metres along the lane
    double tauNow() const { return mTau; }
    bool   onRoad() const { return mOnRoad; }

    void dropout(bool on) { mDropout = on; }
    void badFixes(float prob) { mNoise.badFixProb = prob; }
    // A spell of poor HDOP, wandering in [lo, hi], from now until called with
    // false (whatever the HDOP model).
    void poorHdop(bool on, float lo = 3.0f, float hi = 5.0f);

    float  laneLengthM(uint8_t lane) const;
    double secondsSinceStart() const { return mSimS - mStartS; }
    Trace  finish();

private:
    struct P { double u; double v; };

    double lineRadius() const { return mKerbR + mOffset; }
    double lapLen(double r) const;
    double arcOf(double tau, double r) const;
    double tauOf(double arc, double r) const;
    P      trackPoint(double tau, double r) const;
    P      outwardNormal(double tau) const;
    Oval::Vec2 world(P p) const;

    template <typename Leg> void runLeg(Leg&& leg);
    void   moveAlong(double ds, double v, double dt);
    float  speedNow(const Speed& sp);
    void   emitSample();
    void   scheduleNext();
    uint32_t msAt(double s) const;
    uint32_t activeMsAt(double s) const;
    void   addCommand(CmdKind kind, uint8_t lane);
    double gauss(double sigma);
    float  nextHdop(double dtS);

    Track    mTrack;
    Noise    mNoise;
    std::mt19937 mRng;
    std::mt19937 mHdopRng; // its own stream, so the noise draws do not depend on the HDOP model
    PortableRandom::Normal  mNorm{0.0, 1.0};
    PortableRandom::Uniform mUni{0.0, 1.0};

    double mStraight = 0.0;
    double mKerbR    = 36.5;
    double mCosH = 1.0, mSinH = 0.0;
    // Oval::LocalFrame's projection around the track centre, with the portable
    // cosine: std::cos would make latitudes and longitudes differ in the last
    // bit from one maths library to another.
    struct Frame {
        double lat0 = 0.0, lon0 = 0.0, mPerDegLat = 1.0, mPerDegLon = 1.0;
        void setOrigin(double latDeg, double lonDeg);
        void toLatLon(Oval::Vec2 p, double& latDeg, double& lonDeg) const;
    };
    Frame mFrame;

    // Runner
    uint8_t mLane = 1;
    double  mOffset = 0.30, mTargetOffset = 0.30, mLateralMps = 1.5;
    double  mTau = 0.0, mTauGate = 0.0, mTu = 0.0;
    double  mDetourE = 0.0;
    double  mSpeedFactor = 1.0, mSpeedPhase = 0.0, mCurJitter = 0.0;

    // Clock
    uint32_t mT0 = 0;
    double   mSimS = 0.0, mNextSampleS = 0.0, mStartS = 0.0;
    bool     mStarted = false;
    bool     mPaused = false;
    double   mPausedAccumS = 0.0, mPauseStartS = 0.0;
    bool     mDropout = false;

    // Noise state
    double mDriftX = 0.0, mDriftY = 0.0;
    bool   mHaveHdop = false;
    double mHdopX = 0.0, mHdopLastS = 0.0;   // the slow wander (log HDOP)
    double mSpellLeftS = 0.0, mSpellH = 0.0; // a random spell: time left, HDOP now
    bool   mPoor = false;                    // a scripted spell (poorHdop())
    float  mPoorLo = 3.0f, mPoorHi = 5.0f;
    bool   mHaveDistPos = false;
    double mDistX = 0.0, mDistY = 0.0;
    bool   mHaveTrue = false;
    double mTrueX = 0.0, mTrueY = 0.0;
    double mGpsDist = 0.0;

    // Truth bookkeeping (lane-change formula)
    double mFrac = 0.0, mSegFracStart = 0.0, mCreditBase = 0.0, mCredited = 0.0;
    double mPhys = 0.0, mLapPhysStart = 0.0;
    double mPhysPos = 0.0, mPhysMax = 0.0; // along-line position, and its high-water mark
    bool   mCredit = true;  // false while off the track
    size_t mSegCross0 = 0;  // crossings before the current join or Lap press
    size_t mGatesOff  = 0;  // passes of the start point made off the track since

    // Road
    bool   mOnRoad  = false;
    P      mRoadP{0.0, 0.0};
    double mRoadM   = 0.0;
    double mGpsRoad = 0.0;

    // Path shapes
    std::vector<double> mPathCum; // cumulative length at each path point
    double mPathLen = 0.0;

    Trace mOut;
};

} // namespace TraceGen
