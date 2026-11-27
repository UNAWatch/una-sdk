// Seeded synthetic GPS traces for the Oval host tests. See TraceGen.hpp.

#include "TraceGen.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace TraceGen {

namespace {

constexpr double kPi = 3.141592653589793;

// Lane measurement lines, computed here from the rules rather than taken
// from Oval::LaneTable, so the truth does not depend on the code under test.
double laneOffset(uint8_t lane)
{
    return lane <= 1 ? 0.30 : (lane - 1) * 1.22 + 0.20;
}

// The realistic HDOP's slow wander: log HDOP is a first-order autoregressive
// process (standard deviation 0.2, time constant 2 minutes) around 0.96,
// clamped to 0.7 to 1.5 and given to 0.1 as receivers do. Its root mean
// square is then 1.0 (within 0.01), so the white noise, posSigmaM times HDOP,
// keeps each test's overall noise band.
constexpr double kHdopCentre = 0.96;
constexpr double kHdopLogSd  = 0.2;
constexpr double kHdopTauS   = 120.0;
constexpr double kHdopLo     = 0.7;
constexpr double kHdopHi     = 1.5;

double quantiseHdop(double h)
{
    return std::round(h * 10.0) / 10.0;
}

} // namespace

void Generator::Frame::setOrigin(double latDeg, double lonDeg)
{
    constexpr double kDegToRad = 0.017453292519943295;
    lat0       = latDeg;
    lon0       = lonDeg;
    mPerDegLat = Oval::LocalFrame::kEarthRadiusM * kDegToRad;
    mPerDegLon = mPerDegLat * PortableMath::cos(latDeg * kDegToRad);
}

void Generator::Frame::toLatLon(Oval::Vec2 p, double& latDeg, double& lonDeg) const
{
    latDeg = lat0 + static_cast<double>(p.y) / mPerDegLat;
    lonDeg = lon0 + static_cast<double>(p.x) / mPerDegLon;
    if (lonDeg > 180.0) {
        lonDeg -= 360.0;
    } else if (lonDeg < -180.0) {
        lonDeg += 360.0;
    }
}

Generator::Generator(const Track& track, const Noise& noise, uint32_t seed)
    : mTrack(track), mNoise(noise), mRng(PortableRandom::seeded(seed)),
      mHdopRng(PortableRandom::seeded(seed) ^ 0x5bd1e995u)
{
    mKerbR    = track.kerbRadiusM;
    mStraight = track.straightM > 0.0f
        ? track.straightM
        : (400.0 - 2.0 * kPi * (mKerbR + laneOffset(1))) / 2.0;
    const double h = track.headingDeg * kPi / 180.0;
    mCosH = PortableMath::cos(h);
    mSinH = PortableMath::sin(h);
    mFrame.setOrigin(track.centreLat, track.centreLon);
    mSpeedPhase = 2.0 * kPi * mUni(mRng);

    const size_t n = std::min(track.pathU.size(), track.pathV.size());
    if (n >= 3) {
        mPathCum.resize(n + 1);
        mPathCum[0] = 0.0;
        for (size_t i = 1; i <= n; ++i) {
            const size_t j = i % n;
            mPathCum[i] = mPathCum[i - 1]
                + PortableMath::hypot(track.pathU[j] - track.pathU[i - 1], track.pathV[j] - track.pathV[i - 1]);
        }
        mPathLen = mPathCum[n];
    }
}

double Generator::gauss(double sigma)
{
    return sigma > 0.0 ? sigma * mNorm(mRng) : 0.0;
}

void Generator::poorHdop(bool on, float lo, float hi)
{
    if (on && !mPoor) {
        mSpellH = PortableRandom::uniform(mHdopRng, lo, hi);
    }
    mPoor   = on;
    mPoorLo = lo;
    mPoorHi = hi;
}

float Generator::nextHdop(double dtS)
{
    PortableRandom::Normal  norm(0.0, 1.0);
    PortableRandom::Uniform uni(0.0, 1.0);
    // A spell's HDOP wanders a little within its range.
    auto wander = [&](double lo, double hi) {
        mSpellH = std::min(std::max(mSpellH + 0.25 * norm(mHdopRng), lo), hi);
        return quantiseHdop(mSpellH);
    };

    // The slow wander always runs, so a spell ends where the wander has got to.
    const double a = PortableMath::exp(-std::max(dtS, 0.0) / kHdopTauS);
    mHdopX = mHaveHdop ? a * mHdopX + std::sqrt(1.0 - a * a) * kHdopLogSd * norm(mHdopRng)
                       : kHdopLogSd * norm(mHdopRng);
    mHaveHdop = true;

    if (mPoor) {
        return static_cast<float>(wander(mPoorLo, mPoorHi));
    }
    switch (mNoise.hdopModel) {
    case HdopModel::Unknown:
    case HdopModel::MetresAccuracy: // the white noise as at HDOP 1; emitSample() reports the figure
        return -1.0f;
    case HdopModel::Steady:
        return mNoise.hdopSteady;
    case HdopModel::Realistic:
        break;
    }
    if (mSpellLeftS <= 0.0 && mNoise.hdopSpellsPerHour > 0.0f
        && uni(mHdopRng) < mNoise.hdopSpellsPerHour * dtS / 3600.0) {
        mSpellLeftS = mNoise.hdopSpellMinS + (mNoise.hdopSpellMaxS - mNoise.hdopSpellMinS) * uni(mHdopRng);
        mSpellH     = mNoise.hdopSpellLo + (mNoise.hdopSpellHi - mNoise.hdopSpellLo) * uni(mHdopRng);
    }
    if (mSpellLeftS > 0.0) {
        mSpellLeftS -= dtS;
        return static_cast<float>(wander(mNoise.hdopSpellLo, mNoise.hdopSpellHi));
    }
    const double h = kHdopCentre * PortableMath::exp(mHdopX);
    return static_cast<float>(quantiseHdop(std::min(std::max(h, kHdopLo), kHdopHi)));
}

// ---------------------------------------------------------------------------
// Geometry. Track coordinates (u along the straights, v across), centred on
// the track. tau in [0, 4): 0 home straight, 1 first bend, 2 back straight,
// 3 second bend; the fraction is along the straight or round the bend, so the
// same tau is the same physical point in every lane. Running is anticlockwise.
// ---------------------------------------------------------------------------

double Generator::lapLen(double r) const
{
    if (mPathLen > 0.0) {
        return mPathLen;
    }
    return 2.0 * mStraight + 2.0 * kPi * r;
}

float Generator::laneLengthM(uint8_t lane) const
{
    return static_cast<float>(lapLen(mKerbR + laneOffset(lane)));
}

double Generator::arcOf(double tau, double r) const
{
    if (mPathLen > 0.0) {
        return tau / 4.0 * mPathLen;
    }
    const double lens[4] = {mStraight, kPi * r, mStraight, kPi * r};
    int seg = static_cast<int>(std::floor(tau));
    seg = std::min(std::max(seg, 0), 3);
    double a = 0.0;
    for (int i = 0; i < seg; ++i) {
        a += lens[i];
    }
    return a + (tau - seg) * lens[seg];
}

double Generator::tauOf(double arc, double r) const
{
    const double L = lapLen(r);
    double a = std::fmod(arc, L);
    if (a < 0.0) {
        a += L;
    }
    if (mPathLen > 0.0) {
        return std::min(4.0 * a / L, 3.999999999);
    }
    const double lens[4] = {mStraight, kPi * r, mStraight, kPi * r};
    for (int i = 0; i < 4; ++i) {
        if (a < lens[i] || i == 3) {
            return i + std::min(a / lens[i], 0.999999999);
        }
        a -= lens[i];
    }
    return 0.0;
}

Generator::P Generator::trackPoint(double tau, double r) const
{
    if (mPathLen > 0.0) {
        const double a = std::fmod(std::max(tau, 0.0) / 4.0 * mPathLen, mPathLen);
        const size_t n = mTrack.pathU.size();
        size_t i = static_cast<size_t>(std::upper_bound(mPathCum.begin(), mPathCum.end(), a) - mPathCum.begin());
        i = std::min(std::max<size_t>(i, 1), n);
        const double seg = mPathCum[i] - mPathCum[i - 1];
        const double f   = seg > 0.0 ? (a - mPathCum[i - 1]) / seg : 0.0;
        const size_t j   = i % n;
        return {mTrack.pathU[i - 1] + f * (mTrack.pathU[j] - mTrack.pathU[i - 1]),
                mTrack.pathV[i - 1] + f * (mTrack.pathV[j] - mTrack.pathV[i - 1])};
    }
    const int    seg = std::min(std::max(static_cast<int>(std::floor(tau)), 0), 3);
    const double q   = tau - seg;
    const double hs  = mStraight / 2.0;
    switch (seg) {
    case 0: return {-hs + q * mStraight, -r};
    case 1: return {hs + r * PortableMath::sin(q * kPi), -r * PortableMath::cos(q * kPi)};
    case 2: return {hs - q * mStraight, r};
    default: return {-hs - r * PortableMath::sin(q * kPi), r * PortableMath::cos(q * kPi)};
    }
}

Generator::P Generator::outwardNormal(double tau) const
{
    if (mPathLen > 0.0) {
        // Anticlockwise: outward is to the runner's right.
        const P a = trackPoint(tau, 0.0);
        const P b = trackPoint(tau + 4.0 * 0.5 / mPathLen, 0.0);
        const double du = b.u - a.u, dv = b.v - a.v;
        const double l  = PortableMath::hypot(du, dv);
        return l > 0.0 ? P{dv / l, -du / l} : P{0.0, -1.0};
    }
    const int    seg = std::min(std::max(static_cast<int>(std::floor(tau)), 0), 3);
    const double q   = tau - seg;
    switch (seg) {
    case 0: return {0.0, -1.0};
    case 1: return {PortableMath::sin(q * kPi), -PortableMath::cos(q * kPi)};
    case 2: return {0.0, 1.0};
    default: return {-PortableMath::sin(q * kPi), PortableMath::cos(q * kPi)};
    }
}

Oval::Vec2 Generator::world(P p) const
{
    return {static_cast<float>(p.u * mCosH - p.v * mSinH),
            static_cast<float>(p.u * mSinH + p.v * mCosH)};
}

// ---------------------------------------------------------------------------
// Clock and emission
// ---------------------------------------------------------------------------

uint32_t Generator::msAt(double s) const
{
    return mT0 + static_cast<uint32_t>(std::llround(s * 1000.0));
}

uint32_t Generator::activeMsAt(double s) const
{
    double a = (s - mStartS) - mPausedAccumS - (mPaused ? (s - mPauseStartS) : 0.0);
    if (a < 0.0) {
        a = 0.0;
    }
    return static_cast<uint32_t>(std::llround(a * 1000.0));
}

void Generator::scheduleNext()
{
    double period = mNoise.periodMs;
    if (mNoise.periodJitterMs > 0) {
        period += (2.0 * mUni(mRng) - 1.0) * mNoise.periodJitterMs;
    }
    mNextSampleS += period / 1000.0;
}

void Generator::emitSample()
{
    const P tp = trackPoint(mTau, lineRadius());
    const P n  = outwardNormal(mTau);
    const Oval::Vec2 w = mOnRoad ? world(mRoadP) : world({tp.u + n.u * mDetourE, tp.v + n.v * mDetourE});

    mDriftX += gauss(mNoise.driftStepM);
    mDriftY += gauss(mNoise.driftStepM);
    const bool bad = mUni(mRng) < mNoise.badFixProb;
    // HDOP, from its own stream. A reading's white noise is the noise at
    // HDOP 1 times its HDOP (a bad fix keeps the noise at HDOP 1 and adds
    // its own); with no HDOP it is the noise at HDOP 1.
    const float  hdop = nextHdop(mHaveHdop ? mSimS - mHdopLastS : 0.0);
    mHdopLastS = mSimS;
    const double k = (hdop >= 0.0f && !bad) ? hdop : 1.0;
    // A steady drift after Start, on top of the random walk.
    const double since = mStarted ? std::max(mSimS - mStartS, 0.0) : 0.0;
    const double rampX = mNoise.driftRateEMps * since;
    const double rampY = mNoise.driftRateNMps * since;

    Oval::Sample s;
    s.tMs = msAt(mSimS);
    if (!mDropout) {
        const double bx = bad ? gauss(mNoise.badFixSigmaM) : 0.0;
        const double by = bad ? gauss(mNoise.badFixSigmaM) : 0.0;
        const double sx = (mStarted ? mNoise.shiftEastM : 0.0) + rampX;
        const double sy = (mStarted ? mNoise.shiftNorthM : 0.0) + rampY;
        const Oval::Vec2 rep{static_cast<float>(w.x + mDriftX + gauss(mNoise.posSigmaM) * k + bx + sx),
                             static_cast<float>(w.y + mDriftY + gauss(mNoise.posSigmaM) * k + by + sy)};
        double lat = 0.0, lon = 0.0;
        mFrame.toLatLon(rep, lat, lon);
        if (mNoise.floatCoords) {
            lat = static_cast<double>(static_cast<float>(lat));
            lon = static_cast<double>(static_cast<float>(lon));
        }
        s.fix = true;
        s.lat = lat;
        s.lon = lon;
        s.hdop = bad ? static_cast<float>(quantiseHdop(6.0 + 4.0 * mUni(mRng))) : hdop;
        if (!bad) {
            // The older accuracy figure, drawn every reading so the main stream,
            // and so every trace's noise, stays the same. MetresAccuracy reports it,
            // as a version 1 trace is read: metres divided by 4.
            const float acc = static_cast<float>(mNoise.metresAccBaseM + std::fabs(gauss(mNoise.metresAccJitterM)));
            if (mNoise.hdopModel == HdopModel::MetresAccuracy && !mPoor) {
                s.hdop = acc / 4.0f;
            }
        }

        // The kernel's distance: steps between its own noisy positions.
        if (mStarted) {
            const double dx = w.x + mDriftX + rampX + gauss(mNoise.distJitterM) + bx;
            const double dy = w.y + mDriftY + rampY + gauss(mNoise.distJitterM) + by;
            if (mHaveDistPos) {
                const double d = PortableMath::hypot(dx - mDistX, dy - mDistY) * mNoise.distScale;
                mGpsDist += d;
                mOut.gpsDistM += d;
                if (mOnRoad) {
                    mGpsRoad += d;
                }
            }
            mDistX = dx;
            mDistY = dy;
            mHaveDistPos = true;
        }
    } else {
        s.fix = false;
        s.hdop = -1.0f;
        if (mStarted && mNoise.dropoutDistance && mHaveTrue) {
            const double d = PortableMath::hypot(w.x - mTrueX, w.y - mTrueY) * mNoise.distScale;
            mGpsDist += d;
            mOut.gpsDistM += d;
            mHaveDistPos = false; // the next fix restarts the sum, no chord
        }
    }
    mHaveTrue = true;
    mTrueX = w.x;
    mTrueY = w.y;
    s.gpsDistM = (mStarted && mNoise.gpsDist)
        ? static_cast<float>(mNoise.gpsDistStartM + mGpsDist)
        : std::numeric_limits<float>::quiet_NaN();

    s.altM = static_cast<float>(50.0 + gauss(0.3));
    mOut.samples.push_back(s);
    mOut.altM.push_back(s.altM);
    mOut.trueTrackM.push_back(mCredited + mCreditBase + (mFrac - mSegFracStart) * laneLengthM(mLane));
    mOut.pausedAt.push_back(mPaused ? 1 : 0);
    mOut.offsetE.push_back(static_cast<float>(mDriftX + rampX + (mStarted ? mNoise.shiftEastM : 0.0)));
    mOut.offsetN.push_back(static_cast<float>(mDriftY + rampY + (mStarted ? mNoise.shiftNorthM : 0.0)));
    mOut.truePhysM.push_back(mPhys);
    mOut.onTrack.push_back(mOnRoad ? 0 : 1);
    mOut.trueRoadM.push_back(mRoadM);
    mOut.gpsRoadM.push_back(mGpsRoad);
    mOut.trueTau.push_back(mOnRoad ? -1.0 : mTau);

    // The runner's pace wobbles from one reading to the next.
    mSpeedFactor = mCurJitter > 0.0 ? std::min(std::max(1.0 + gauss(mCurJitter), 0.6), 1.4) : 1.0;
}

template <typename Leg>
void Generator::runLeg(Leg&& leg)
{
    for (;;) {
        if (mSimS >= mNextSampleS - 1e-9) {
            emitSample();
            scheduleNext();
            continue;
        }
        const double maxDt = std::min(0.2, mNextSampleS - mSimS);
        double used = 0.0;
        const bool done = leg(maxDt, used);
        mSimS += used;
        if (done) {
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// Motion
// ---------------------------------------------------------------------------

float Generator::speedNow(const Speed& sp)
{
    const double t     = mSimS - mStartS;
    const bool   bend  = (static_cast<int>(std::floor(mTau)) % 2) == 1;
    double v = sp.baseMps * (1.0 + sp.variation * PortableMath::sin(2.0 * kPi * t / sp.periodS + mSpeedPhase));
    if (bend) {
        v *= sp.bendFactor;
    }
    v *= mSpeedFactor;
    return static_cast<float>(std::max(v, 0.3));
}

void Generator::moveAlong(double ds, double v, double dt)
{
    if (mOffset != mTargetOffset) {
        const double step = mLateralMps * dt;
        if (std::fabs(mTargetOffset - mOffset) <= step) {
            mOffset = mTargetOffset;
        } else {
            mOffset += (mTargetOffset > mOffset) ? step : -step;
        }
    }
    const double r      = lineRadius();
    const double L       = lapLen(r);
    const double a0      = arcOf(mTau, r);
    const double tauNew  = tauOf(a0 + ds, r);
    double dTau = tauNew - mTau;
    if (dTau > 2.0) {
        dTau -= 4.0;
    } else if (dTau < -2.0) {
        dTau += 4.0;
    }

    if (!mCredit) {
        // Off the track: the runner goes round the outside, which is not
        // track distance and does not end a lap.
        mTu += dTau;
        mTau = tauNew;
        return;
    }
    const double laneLen = laneLengthM(mLane);
    bool crossed = false;
    if (ds > 0.0) {
        // Lap k ends the first time the unwrapped position passes the start
        // point for the k-th time; going back and forth over it does not.
        const double thr = mTauGate + 4.0 * static_cast<double>(mOut.crossMs.size() - mSegCross0 + 1);
        if (mTu < thr && mTu + dTau >= thr) {
            double toGate = arcOf(std::fmod(mTauGate, 4.0), r) - a0;
            while (toGate < 0.0) {
                toGate += L;
            }
            toGate = std::min(toGate, ds);
            const double tc = mSimS + toGate / v;
            const double credit = mCreditBase + (1.0 - mSegFracStart) * laneLen;
            mOut.crossMs.push_back(msAt(tc));
            mOut.crossActiveMs.push_back(activeMsAt(tc));
            mOut.lapCreditM.push_back(credit);
            const double physAtCross = mPhys + (mPaused ? 0.0 : std::max(0.0, mPhysPos + toGate - mPhysMax));
            mOut.lapPhysM.push_back(physAtCross - mLapPhysStart);
            mLapPhysStart = physAtCross;
            mCredited    += credit;
            mCreditBase   = 0.0;
            mSegFracStart = 0.0;
            mFrac         = mPaused ? 0.0 : (ds - toGate) / laneLen;
            crossed = true;
        }
    }
    if (!crossed && !mPaused) {
        mFrac += ds / laneLen;
    }
    if (!mPaused && mStarted) {
        mPhysPos += ds;
        if (mPhysPos > mPhysMax) {
            mPhys   += mPhysPos - mPhysMax;
            mPhysMax = mPhysPos;
        }
    }
    mTu += dTau;
    mTau = tauNew;
    if (mStarted) {
        mOut.movedM += std::fabs(ds);
    }
}

void Generator::begin(float startFrac, uint8_t lane, uint32_t t0Ms, int primeReadings)
{
    mLane = lane;
    mOffset = mTargetOffset = laneOffset(lane);
    const double r = lineRadius();
    mTau = mTauGate = mTu = tauOf(startFrac * lapLen(r), r);
    mFrame.toLatLon(world(trackPoint(mTau, r)), mOut.startLat, mOut.startLon);
    mT0 = t0Ms;
    mSimS = mNextSampleS = 0.0;
    for (int i = 0; i < primeReadings; ++i) {
        mSimS = mNextSampleS;
        emitSample();
        scheduleNext();
    }
    mStartS  = primeReadings > 0 ? mSimS + 0.5 : 0.0;
    mSimS    = mStartS;
    mStarted = true;
    mOut.startIndex = mOut.samples.size();
    mOut.startMs    = msAt(mStartS);
    mOut.startLane  = lane;
}

void Generator::beginRolling(float startFrac, uint8_t lane, const Speed& sp, double runInM, uint32_t t0Ms)
{
    mLane = lane;
    mOffset = mTargetOffset = laneOffset(lane);
    const double r = lineRadius();
    const double L = lapLen(r);
    double from = std::fmod(startFrac * L - runInM, L);
    if (from < 0.0) {
        from += L;
    }
    mTauGate = tauOf(startFrac * L, r);
    mTau     = tauOf(from, r);
    double back = mTauGate - mTau;
    if (back < 0.0) {
        back += 4.0;
    }
    mTu = mTauGate - back; // unwrapped, so the run-in never ends a lap
    mFrame.toLatLon(world(trackPoint(mTauGate, r)), mOut.startLat, mOut.startLon);
    mT0 = t0Ms;
    mSimS = mNextSampleS = 0.0;
    mStartS = 0.0;

    // The run-in: readings as they fall due, none of it truth.
    double remaining = runInM;
    mCurJitter = sp.jitter;
    runLeg([&](double maxDt, double& used) {
        if (remaining <= 1e-9) {
            return true;
        }
        const double v  = speedNow(sp);
        const double dt = std::min(maxDt, remaining / v);
        const double ds = v * dt;
        moveAlong(ds, v, dt);
        remaining -= ds;
        used = dt;
        return remaining <= 1e-9;
    });
    mCurJitter   = 0.0;
    mSpeedFactor = 1.0;

    // Start, at the start point and on the move.
    mTau     = mTauGate;
    mTu      = mTauGate;
    mFrac    = 0.0;
    mStartS  = mSimS;
    mStarted = true;
    mOut.startIndex = mOut.samples.size();
    mOut.startMs    = msAt(mStartS);
    mOut.startLane  = lane;
}

void Generator::run(double metres, const Speed& sp)
{
    double remaining = metres;
    mCurJitter = sp.jitter;
    runLeg([&](double maxDt, double& used) {
        if (remaining <= 1e-9) {
            return true;
        }
        const double v  = speedNow(sp);
        const double dt = std::min(maxDt, remaining / v);
        const double ds = v * dt;
        moveAlong(ds, v, dt);
        remaining -= ds;
        used = dt;
        return remaining <= 1e-9;
    });
    mCurJitter   = 0.0;
    mSpeedFactor = 1.0;
}

void Generator::runLaps(double laps, const Speed& sp)
{
    run(laps * laneLengthM(mLane), sp);
}

void Generator::stand(double seconds)
{
    double remaining = seconds;
    runLeg([&](double maxDt, double& used) {
        if (remaining <= 1e-9) {
            return true;
        }
        used = std::min(maxDt, remaining);
        remaining -= used;
        return remaining <= 1e-9;
    });
}

void Generator::shuttle(double seconds, float amplitudeM, float speedMps, bool backwardFirst)
{
    double remaining = seconds;
    double pos = 0.0;
    int    dir = backwardFirst ? -1 : 1;
    const double v = speedMps;
    runLeg([&](double maxDt, double& used) {
        if (remaining <= 1e-9) {
            return true;
        }
        double toBound = dir > 0 ? (amplitudeM - pos) / v : (pos + amplitudeM) / v;
        if (toBound <= 1e-9) {
            dir = -dir;
            toBound = 2.0 * amplitudeM / v;
        }
        const double dt = std::min(std::min(maxDt, remaining), toBound);
        moveAlong(dir * v * dt, v, dt);
        pos += dir * v * dt;
        remaining -= dt;
        used = dt;
        return remaining <= 1e-9;
    });
}

void Generator::detour(float outM, float speedMps)
{
    bool out = true;
    const double v = speedMps;
    runLeg([&](double maxDt, double& used) {
        if (out) {
            const double dt = std::min(maxDt, (outM - mDetourE) / v);
            mDetourE += v * dt;
            used = dt;
            if (mDetourE >= outM - 1e-9) {
                mDetourE = outM;
                out = false;
            }
            return false;
        }
        const double dt = std::min(maxDt, mDetourE / v);
        mDetourE -= v * dt;
        used = dt;
        if (mDetourE <= 1e-9) {
            mDetourE = 0.0;
            return true;
        }
        return false;
    });
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

void Generator::addCommand(CmdKind kind, uint8_t lane)
{
    Command c;
    c.beforeSample = mOut.samples.size();
    c.kind         = kind;
    c.tMs          = msAt(mSimS);
    c.lane         = lane;
    mOut.commands.push_back(c);
}

void Generator::changeLane(uint8_t lane, float lateralMps, bool declare)
{
    if (declare) {
        addCommand(CmdKind::SetLane, lane);
        // The lane-change formula: what is run so far stays at the old lane's length.
        mCreditBase  += (mFrac - mSegFracStart) * laneLengthM(mLane);
        mSegFracStart = mFrac;
        mLane         = lane;
    }
    mTargetOffset = laneOffset(lane);
    mLateralMps   = lateralMps;
}

void Generator::pause()
{
    if (mPaused) {
        return;
    }
    addCommand(CmdKind::Pause, 0);
    mPaused      = true;
    mPauseStartS = mSimS;
}

void Generator::resume()
{
    if (!mPaused) {
        return;
    }
    addCommand(CmdKind::Resume, 0);
    mPausedAccumS += mSimS - mPauseStartS;
    mPaused = false;
}

void Generator::leaveTrack(float outM, float speedMps, double aheadM)
{
    const double v = speedMps;
    // Out, along the outward normal.
    runLeg([&](double maxDt, double& used) {
        const double dt = std::min(maxDt, (outM - mDetourE) / v);
        mDetourE += v * dt;
        used = dt;
        if (mDetourE >= outM - 1e-9) {
            mDetourE = outM;
            return true;
        }
        return false;
    });
    // Round the outside, not on the track.
    mCredit = false;
    double remaining = aheadM;
    runLeg([&](double maxDt, double& used) {
        if (remaining <= 1e-9) {
            return true;
        }
        const double dt = std::min(maxDt, remaining / v);
        moveAlong(v * dt, v, dt);
        remaining -= v * dt;
        used = dt;
        return remaining <= 1e-9;
    });
    mCredit = true;
    // Back in.
    runLeg([&](double maxDt, double& used) {
        const double dt = std::min(maxDt, mDetourE / v);
        mDetourE -= v * dt;
        used = dt;
        if (mDetourE <= 1e-9) {
            mDetourE = 0.0;
            return true;
        }
        return false;
    });
}

// ---------------------------------------------------------------------------
// Non-standard shapes
// ---------------------------------------------------------------------------

namespace {

// Scales a closed path about the origin so its length is lapM.
void scaleTo(Track& t, double lapM)
{
    double len = 0.0;
    const size_t n = t.pathU.size();
    for (size_t i = 0; i < n; ++i) {
        const size_t j = (i + 1) % n;
        len += PortableMath::hypot(t.pathU[j] - t.pathU[i], t.pathV[j] - t.pathV[i]);
    }
    const double k = lapM / len;
    for (size_t i = 0; i < n; ++i) {
        t.pathU[i] *= k;
        t.pathV[i] *= k;
    }
}

} // namespace

Track circleTrack(double lapM)
{
    Track t;
    const int    n = 1440;
    const double r = lapM / (2.0 * kPi);
    for (int i = 0; i < n; ++i) {
        const double a = -kPi / 2.0 + 2.0 * kPi * i / n;
        t.pathU.push_back(r * PortableMath::cos(a));
        t.pathV.push_back(r * PortableMath::sin(a));
    }
    scaleTo(t, lapM);
    return t;
}

Track parkLoopTrack(double lapM)
{
    // An elongated, lumpy loop: low harmonics on an oval of about the right
    // proportions.
    Track t;
    const int n = 1440;
    for (int i = 0; i < n; ++i) {
        const double a = -kPi / 2.0 + 2.0 * kPi * i / n;
        const double r = 1.0 + 0.25 * PortableMath::cos(2.0 * a) + 0.08 * PortableMath::sin(3.0 * a) + 0.06 * PortableMath::cos(5.0 * a + 1.0);
        t.pathU.push_back(r * PortableMath::cos(a));
        t.pathV.push_back(r * PortableMath::sin(a));
    }
    scaleTo(t, lapM);
    return t;
}

Track doubleBendTrack()
{
    // Each bend is three arcs, measured on lane 1's line (0.30 m out): 60
    // degrees at 51.543 m, 60 at 34.000 m, 60 at 51.543 m. The straights make
    // the lap exactly 400 m.
    const double r1 = 51.543 + 0.30, r2 = 34.000 + 0.30, a = kPi / 3.0;
    const double bend     = 2.0 * r1 * a + r2 * (kPi - 2.0 * a);
    const double straight = (400.0 - 2.0 * bend) / 2.0;
    const double halfW    = r1 - (r1 - r2) * PortableMath::cos(a);
    Track t;
    double u = -straight / 2.0, v = -halfW, h = 0.0;
    const double step = 0.25;
    auto go = [&](double len, double radius) {
        const int    k  = static_cast<int>(std::ceil(len / step));
        const double ds = len / k;
        for (int i = 0; i < k; ++i) {
            t.pathU.push_back(u);
            t.pathV.push_back(v);
            if (radius > 0.0) {
                const double dh = ds / radius;
                u += radius * (PortableMath::sin(h + dh) - PortableMath::sin(h));
                v -= radius * (PortableMath::cos(h + dh) - PortableMath::cos(h));
                h += dh;
            } else {
                u += ds * PortableMath::cos(h);
                v += ds * PortableMath::sin(h);
            }
        }
    };
    for (int side = 0; side < 2; ++side) {
        go(straight, 0.0);
        go(r1 * a, r1);
        go(r2 * (kPi - 2.0 * a), r2);
        go(r1 * a, r1);
    }
    return t;
}

Track shortTrack(double lane1M)
{
    Track t;
    t.kerbRadiusM = 36.5f;
    t.straightM   = static_cast<float>((lane1M - 2.0 * kPi * (36.5 + laneOffset(1))) / 2.0);
    return t;
}

// ---------------------------------------------------------------------------
// Road and track in one run
// ---------------------------------------------------------------------------

Pt Generator::trackPointAt(double tau, uint8_t lane, double outwardM) const
{
    const double r  = mKerbR + laneOffset(lane);
    const P      tp = trackPoint(tau, r);
    const P      n  = outwardNormal(tau);
    return {tp.u + n.u * outwardM, tp.v + n.v * outwardM};
}

Pt Generator::here() const
{
    if (mOnRoad) {
        return {mRoadP.u, mRoadP.v};
    }
    const P tp = trackPoint(mTau, lineRadius());
    const P n  = outwardNormal(mTau);
    return {tp.u + n.u * mDetourE, tp.v + n.v * mDetourE};
}

double Generator::tauAfter(double tau, uint8_t lane, double metres) const
{
    const double r = mKerbR + laneOffset(lane);
    return tauOf(arcOf(std::fmod(tau, 4.0), r) + metres, r);
}

void Generator::beginAt(Pt start, uint8_t lane, uint32_t t0Ms, int primeReadings)
{
    mOnRoad = true;
    mRoadP  = {start.u, start.v};
    mLane   = lane != 0 ? lane : 1; // "No lane": the road, lane 1's line for the geometry
    mOffset = mTargetOffset = laneOffset(mLane);
    mTau = mTauGate = mTu = 0.0;
    mFrame.toLatLon(world(mRoadP), mOut.startLat, mOut.startLon);
    mT0 = t0Ms;
    mSimS = mNextSampleS = 0.0;
    for (int i = 0; i < primeReadings; ++i) {
        mSimS = mNextSampleS;
        emitSample();
        scheduleNext();
    }
    mStartS  = primeReadings > 0 ? mSimS + 0.5 : 0.0;
    mSimS    = mStartS;
    mStarted = true;
    mOut.startIndex = mOut.samples.size();
    mOut.startMs    = msAt(mStartS);
    mOut.startLane  = lane;
}

void Generator::road(const std::vector<Pt>& route, const Speed& sp)
{
    mCurJitter = sp.jitter;
    for (const Pt& target : route) {
        runLeg([&](double maxDt, double& used) {
            const double du = target.u - mRoadP.u, dv = target.v - mRoadP.v;
            const double remaining = PortableMath::hypot(du, dv);
            if (remaining <= 1e-9) {
                return true;
            }
            const double t = mSimS - mStartS;
            double v = sp.baseMps * (1.0 + sp.variation * PortableMath::sin(2.0 * kPi * t / sp.periodS + mSpeedPhase));
            v = std::max(v * mSpeedFactor, 0.3);
            const double dt = std::min(maxDt, remaining / v);
            const double ds = v * dt;
            mRoadP.u += du / remaining * ds;
            mRoadP.v += dv / remaining * ds;
            if (mStarted && !mPaused) {
                mRoadM += ds;
            }
            used = dt;
            return ds >= remaining - 1e-9;
        });
    }
    mCurJitter   = 0.0;
    mSpeedFactor = 1.0;
}

void Generator::joinTrack(double tau, uint8_t lane)
{
    mOnRoad = false;
    mLane   = lane;
    mOffset = mTargetOffset = laneOffset(lane);
    mTau = mTu = mTauGate = tau;
    mSegCross0    = mOut.crossMs.size();
    mFrac         = 0.0;
    mSegFracStart = 0.0;
    mCreditBase   = 0.0;
    mDetourE      = 0.0;
    mPhysPos      = mPhysMax;
    mLapPhysStart = mPhys;
    TrackVisit v;
    v.joinMs     = msAt(mSimS);
    v.joinTau    = tau;
    v.firstLap   = mOut.crossMs.size();
    v.joinSample = mOut.samples.size();
    mOut.visits.push_back(v);
}

void Generator::leaveToRoad()
{
    const Pt h = here();
    mRoadP  = {h.u, h.v};
    mOnRoad = true;
    // The part-lap run stays as distance.
    const double part = mCreditBase + (mFrac - mSegFracStart) * laneLengthM(mLane);
    mCredited    += part;
    mCreditBase   = 0.0;
    mFrac         = 0.0;
    mSegFracStart = 0.0;
    mDetourE      = 0.0;
    if (!mOut.visits.empty()) {
        TrackVisit& v = mOut.visits.back();
        v.exitMs     = msAt(mSimS);
        v.laps       = mOut.crossMs.size() - v.firstLap;
        v.partM      = part;
        v.exitSample = mOut.samples.size();
    }
}

void Generator::setLane(uint8_t lane)
{
    addCommand(CmdKind::SetLane, lane);
}

void Generator::lapPress()
{
    addCommand(CmdKind::LapPress, 0);
    // The lap so far stays as distance; the next lap ends a lap from here.
    mCredited    += mCreditBase + (mFrac - mSegFracStart) * laneLengthM(mLane);
    mCreditBase   = 0.0;
    mFrac         = 0.0;
    mSegFracStart = 0.0;
    mTauGate      = mTu;
    mSegCross0    = mOut.crossMs.size();
    mLapPhysStart = mPhys;
}

Trace Generator::finish()
{
    if (!mOut.visits.empty() && mOut.visits.back().exitMs == 0) {
        mOut.visits.back().laps = mOut.crossMs.size() - mOut.visits.back().firstLap;
    }
    return mOut;
}

} // namespace TraceGen
