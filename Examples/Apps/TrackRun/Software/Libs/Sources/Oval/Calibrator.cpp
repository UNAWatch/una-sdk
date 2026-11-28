/*******************************************************************************
 * @file   Calibrator.cpp
 * @brief  Oval: the template fit. See Calibrator.hpp and docs/OVAL.md.
 ******************************************************************************/

#include "Oval/Calibrator.hpp"

#include <cmath>

namespace Oval {

namespace {

constexpr float kPi     = 3.14159265358979f;
constexpr float kTwoPi  = 6.28318530717959f;
constexpr float kRadDeg = 57.2957795130823f;

constexpr uint8_t kCoverageBins = 36; // angle bins round the centroid
constexpr uint8_t kStationBins  = 24; // six per straight or bend, for the shape check
constexpr uint8_t kCoverBins    = 12; // three per straight or bend, for the coverage check
constexpr uint8_t kCoverageStep = 8;  // coverage is worked out every this many points
constexpr uint8_t kMaxIterations = 60;
constexpr float   kHuberK        = 1.345f;
constexpr float   kMinScaleM     = 0.3f;
constexpr float   kInlierSigmas  = 3.0f;
constexpr float   kEdgeM         = 0.05f;

inline float wrapPi(float a) noexcept
{
    while (a > kPi) {
        a -= kTwoPi;
    }
    while (a < -kPi) {
        a += kTwoPi;
    }
    return a;
}

/** Residual of one point against the template, and its gradient with respect
 *  to (centre x, centre y, heading, kerb radius, runner offset). */
struct Eval {
    float r;
    float j[5];
};

inline Eval evaluate(float cx, float cy, float c, float s, float kerbR, float runner,
                     float x, float y, float lineOffset, float halfVar = 0.0f) noexcept
{
    const float dx = x - cx;
    const float dy = y - cy;
    const float u  = c * dx + s * dy;
    const float v  = -s * dx + c * dy;
    // Half the straight: (400 - 2 pi (R + 0.30)) / 4.
    const float h = 100.0f - 0.5f * kPi * (kerbR + TrackModel::kLane1LineM);

    float d = 0.0f, gu = 0.0f, gv = 0.0f, gR = -1.0f;
    const float au = std::fabs(u);
    if (au <= h) {
        d  = std::fabs(v) - kerbR;
        gv = v < 0.0f ? -1.0f : 1.0f;
    } else {
        const float w   = au - h;
        float       rho = std::sqrt(w * w + v * v);
        if (rho < 1e-3f) {
            rho = 1e-3f;
        }
        // Noise pushes points round a bend outward on average, by about
        // variance / (2 x radius): taken off, so it does not bias R.
        d  = rho - kerbR - halfVar / rho;
        gu = (u < 0.0f ? -1.0f : 1.0f) * w / rho;
        gv = v / rho;
        // A larger R shortens the straights, moving the bend centre in.
        gR = -1.0f + 0.5f * kPi * w / rho;
    }
    Eval e;
    e.r    = d - lineOffset - runner;
    e.j[0] = -gu * c + gv * s;
    e.j[1] = -gu * s - gv * c;
    e.j[2] = gu * v - gv * u;
    e.j[3] = gR;
    e.j[4] = -1.0f;
    return e;
}

inline float huberWeight(float r, float k) noexcept
{
    const float a = std::fabs(r);
    return a <= k ? 1.0f : k / a;
}

/** Solves (A) x = b for a symmetric positive definite 5x5 A by Cholesky.
 *  False if A is not positive definite. */
bool solve5(const float A[5][5], const float b[5], float x[5]) noexcept
{
    float L[5][5] = {};
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j <= i; ++j) {
            float sum = A[i][j];
            for (int k = 0; k < j; ++k) {
                sum -= L[i][k] * L[j][k];
            }
            if (i == j) {
                if (!(sum > 0.0f)) {
                    return false;
                }
                L[i][i] = std::sqrt(sum);
            } else {
                L[i][j] = sum / L[j][j];
            }
        }
    }
    float y[5];
    for (int i = 0; i < 5; ++i) {
        float sum = b[i];
        for (int k = 0; k < i; ++k) {
            sum -= L[i][k] * y[k];
        }
        y[i] = sum / L[i][i];
    }
    for (int i = 4; i >= 0; --i) {
        float sum = y[i];
        for (int k = i + 1; k < 5; ++k) {
            sum -= L[k][i] * x[k];
        }
        x[i] = sum / L[i][i];
    }
    return true;
}

} // namespace

Calibrator::Calibrator(const Config& cfg) noexcept
    : mCfg(cfg)
{
    reset(LaneTable());
}

void Calibrator::setConfig(const Config& cfg) noexcept
{
    mCfg = cfg;
    reset(mTable);
}

void Calibrator::reset(const LaneTable& table) noexcept
{
    mTable      = table;
    mCount      = 0;
    mSpacingM   = mCfg.calSpacingM > 0.1f ? mCfg.calSpacingM : 0.1f;
    mHaveLast   = false;
    mLast       = Vec2();
    mCovChecked = 0;
    mLaps       = 0;
    mClockwise  = false;
    mAltSum     = 0.0;
    mAltCount   = 0;
    mStage      = Stage::Collecting;
    mReject     = Reject::None;
    mHavePrev   = false;
    mPrev       = Params();
    mModel.clear();
    mFit        = FitInfo();
}

Vec2 Calibrator::pointAt(uint16_t i) const noexcept
{
    return {0.1f * static_cast<float>(mPts[i].x), 0.1f * static_cast<float>(mPts[i].y)};
}

// ---------------------------------------------------------------------------
// Collecting
// ---------------------------------------------------------------------------

float Calibrator::pointWeight(uint16_t i) const noexcept
{
    const uint8_t h = mPts[i].hdop20;
    return h == 0 ? 1.0f : hdopWeight(0.05f * static_cast<float>(h), mCfg);
}

float Calibrator::pointHdop(uint16_t i) const noexcept
{
    const uint8_t h = mPts[i].hdop20;
    return h == 0 ? -1.0f : 0.05f * static_cast<float>(h);
}

Calibrator::Change Calibrator::add(Vec2 p, uint8_t lane, float altM, float hdop) noexcept
{
    if (isFinal() || !mTable.isValidLane(lane)) {
        return Change::None;
    }
    // Decimetres in an int16: anything over 3 km from the start point is not
    // on this track.
    if (!(std::fabs(p.x) < 3000.0f) || !(std::fabs(p.y) < 3000.0f)) {
        return Change::None;
    }
    if (std::isfinite(altM)) {
        mAltSum += altM;
        ++mAltCount;
    }
    if (mHaveLast && length(p - mLast) < mSpacingM) {
        return Change::None;
    }
    if (mCount >= kMaxPoints) {
        thin();
    }
    Point& q = mPts[mCount++];
    q.x    = static_cast<int16_t>(std::lround(p.x * 10.0f));
    q.y    = static_cast<int16_t>(std::lround(p.y * 10.0f));
    q.lane = lane;
    if (hdop >= 0.0f) {
        const long h = std::lround(hdop * 20.0f);
        q.hdop20 = static_cast<uint8_t>(h < 1 ? 1 : (h > 255 ? 255 : h));
    } else {
        q.hdop20 = 0;
    }
    mLast     = p;
    mHaveLast = true;

    if (mCount - mCovChecked < kCoverageStep) {
        return Change::None;
    }
    const uint8_t before = mLaps;
    updateCoverage();
    if (mLaps > before && mLaps >= mCfg.calProvisionalLaps) {
        return runFit();
    }
    return Change::None;
}

void Calibrator::thin() noexcept
{
    // Keep every other point: the buffer stays spread evenly over the whole
    // run, and later points are stored twice as far apart.
    uint16_t n = 0;
    for (uint16_t i = 0; i < mCount; i += 2) {
        mPts[n++] = mPts[i];
    }
    mCount      = n;
    mCovChecked = n;
    mSpacingM  *= 2.0f;
}

void Calibrator::updateCoverage() noexcept
{
    mCovChecked = mCount;
    if (mCount < 3) {
        return;
    }
    double sx = 0.0, sy = 0.0;
    for (uint16_t i = 0; i < mCount; ++i) {
        const Vec2 p = pointAt(i);
        sx += p.x;
        sy += p.y;
    }
    const Vec2 c{static_cast<float>(sx / mCount), static_cast<float>(sy / mCount)};

    // The angle swept round the centroid, point to point. Each time round
    // a closed loop adds a full turn, whatever its shape; going back and
    // forth cancels out.
    uint64_t bins  = 0;
    float    sweep = 0.0f;
    float    prev  = 0.0f;
    for (uint16_t i = 0; i < mCount; ++i) {
        const Vec2  d = pointAt(i) - c;
        const float a = std::atan2(d.y, d.x);
        int b = static_cast<int>((a + kPi) / kTwoPi * kCoverageBins);
        b = b < 0 ? 0 : (b >= kCoverageBins ? kCoverageBins - 1 : b);
        bins |= (uint64_t(1) << b);
        if (i > 0) {
            sweep += wrapPi(a - prev);
        }
        prev = a;
    }
    const bool all = bins == ((uint64_t(1) << kCoverageBins) - 1);
    const int  laps = all ? static_cast<int>(std::fabs(sweep) / kTwoPi) : 0;
    if (laps > mLaps) {
        mLaps = static_cast<uint8_t>(laps > 255 ? 255 : laps);
    }
    mClockwise = sweep < 0.0f;
}

// ---------------------------------------------------------------------------
// Fitting
// ---------------------------------------------------------------------------

float Calibrator::residualScale(const Params& p) const noexcept
{
    // 1.4826 x the median absolute residual, from a histogram so no buffer
    // of residuals is needed: 0.2 m bins to 25.6 m.
    constexpr int   kBins = 128;
    constexpr float kBinM = 0.2f;
    uint16_t hist[kBins] = {};
    const float c = std::cos(p.heading), s = std::sin(p.heading);
    for (uint16_t i = 0; i < mCount; ++i) {
        const Vec2 q = pointAt(i);
        const Eval e = evaluate(p.cx, p.cy, c, s, p.kerbR, p.runner, q.x, q.y,
                                mTable.lineOffsetM(mPts[i].lane));
        int b = static_cast<int>(std::fabs(e.r) / kBinM);
        hist[b < kBins ? b : kBins - 1]++;
    }
    const float half = 0.5f * static_cast<float>(mCount);
    float cum = 0.0f;
    for (int b = 0; b < kBins; ++b) {
        const float n = static_cast<float>(hist[b]);
        if (cum + n >= half && n > 0.0f) {
            const float median = (static_cast<float>(b) + (half - cum) / n) * kBinM;
            return 1.4826f * median;
        }
        cum += n;
    }
    return 1.4826f * kBins * kBinM;
}

float Calibrator::scaleFor(const TrackModel& m, float runnerM) const noexcept
{
    if (!m.isValid() || mCount == 0) {
        return 0.0f;
    }
    Params p;
    p.cx      = m.centre().x;
    p.cy      = m.centre().y;
    p.heading = m.headingRad();
    p.kerbR   = m.kerbRadiusM();
    p.runner  = runnerM;
    const float scale = residualScale(p);
    return scale < kMinScaleM ? kMinScaleM : scale;
}

bool Calibrator::fit(Params& p, uint8_t& iterations) const noexcept
{
    const float rLo = mCfg.fitRadiusMinM;
    const float rHi = mCfg.fitRadiusMaxM;
    float lambda = 1e-3f;
    iterations = 0;

    for (uint8_t it = 0; it < kMaxIterations; ++it) {
        iterations = static_cast<uint8_t>(it + 1);
        float scale = residualScale(p);
        if (scale < kMinScaleM) {
            scale = kMinScaleM;
        }
        const float k = kHuberK * scale;
        const float halfVar = 0.5f * scale * scale;
        const float c = std::cos(p.heading), s = std::sin(p.heading);

        // Normal equations with Huber weights at the current parameters, times
        // each point's weight from its HDOP.
        float A[5][5] = {};
        float g[5]    = {};
        float cost0   = 0.0f;
        for (uint16_t i = 0; i < mCount; ++i) {
            const Vec2  q  = pointAt(i);
            const Eval  e  = evaluate(p.cx, p.cy, c, s, p.kerbR, p.runner, q.x, q.y,
                                      mTable.lineOffsetM(mPts[i].lane), halfVar);
            const float w  = huberWeight(e.r, k) * pointWeight(i);
            cost0 += w * e.r * e.r;
            for (int a = 0; a < 5; ++a) {
                g[a] += w * e.j[a] * e.r;
                for (int b = 0; b <= a; ++b) {
                    A[a][b] += w * e.j[a] * e.j[b];
                }
            }
        }
        for (int a = 0; a < 5; ++a) {
            for (int b = a + 1; b < 5; ++b) {
                A[a][b] = A[b][a];
            }
        }

        // Levenberg-Marquardt step: raise the damping until the weighted cost
        // goes down.
        bool  accepted = false;
        Params next    = p;
        for (int tries = 0; tries < 10 && !accepted; ++tries) {
            float D[5][5];
            for (int a = 0; a < 5; ++a) {
                for (int b = 0; b < 5; ++b) {
                    D[a][b] = A[a][b];
                }
                D[a][a] += lambda * A[a][a] + 1e-6f;
            }
            float negG[5], step[5];
            for (int a = 0; a < 5; ++a) {
                negG[a] = -g[a];
            }
            if (!solve5(D, negG, step)) {
                lambda *= 10.0f;
                continue;
            }
            next.cx      = p.cx + step[0];
            next.cy      = p.cy + step[1];
            next.heading = p.heading + step[2];
            next.kerbR   = p.kerbR + step[3];
            next.runner  = p.runner + step[4];
            next.kerbR   = next.kerbR < rLo ? rLo : (next.kerbR > rHi ? rHi : next.kerbR);

            // Cost of the step with the same weights.
            const float c1 = std::cos(next.heading), s1 = std::sin(next.heading);
            float cost1 = 0.0f;
            for (uint16_t i = 0; i < mCount; ++i) {
                const Vec2  q  = pointAt(i);
                const float o  = mTable.lineOffsetM(mPts[i].lane);
                const Eval  e0 = evaluate(p.cx, p.cy, c, s, p.kerbR, p.runner, q.x, q.y, o, halfVar);
                const Eval  e1 = evaluate(next.cx, next.cy, c1, s1, next.kerbR, next.runner, q.x, q.y, o, halfVar);
                cost1 += huberWeight(e0.r, k) * pointWeight(i) * e1.r * e1.r;
            }
            if (cost1 <= cost0) {
                accepted = true;
                lambda   = lambda * 0.3f < 1e-6f ? 1e-6f : lambda * 0.3f;
            } else {
                lambda *= 4.0f;
            }
        }
        if (!accepted) {
            break; // no step lowers the cost: converged
        }
        const float moved = std::fabs(next.cx - p.cx) + std::fabs(next.cy - p.cy)
            + std::fabs(next.kerbR - p.kerbR) + std::fabs(next.runner - p.runner)
            + 50.0f * std::fabs(next.heading - p.heading);
        p = next;
        if (moved < 1e-3f) {
            break;
        }
    }
    return std::isfinite(p.cx) && std::isfinite(p.cy) && std::isfinite(p.heading)
        && std::isfinite(p.kerbR) && std::isfinite(p.runner);
}

void Calibrator::assess(const Params& p, FitInfo& info) const noexcept
{
    float scale = residualScale(p);
    if (scale < kMinScaleM) {
        scale = kMinScaleM;
    }
    info.scaleM      = scale;
    info.kerbRadiusM = p.kerbR;
    info.runnerM     = p.runner;

    // Residuals binned by station along the fitted track. The bin means
    // show the shape mismatch; noise alone gives each bin mean a variance of
    // (spread in the bin) / n, which is taken off, so the figure does not
    // grow with GPS noise.
    TrackModel m;
    m.set({p.cx, p.cy}, p.heading, p.kerbR, mClockwise);
    uint16_t n[kStationBins]    = {};
    uint16_t cover[kCoverBins]  = {};
    float    sum[kStationBins]  = {};
    float    sum2[kStationBins] = {};
    const float limit = kInlierSigmas * scale;
    for (uint16_t i = 0; i < mCount; ++i) {
        const float o = mTable.lineOffsetM(mPts[i].lane);
        const TrackModel::Projection pr = m.project(pointAt(i), o);
        float r = pr.lateralM - p.runner;
        if (static_cast<int>(pr.tau) % 2 == 1) {
            r -= 0.5f * scale * scale / (p.kerbR + o + pr.lateralM);
        }
        if (std::fabs(r) > limit) {
            continue;
        }
        int b = static_cast<int>(pr.tau * (kStationBins / 4));
        b = b < 0 ? 0 : (b >= kStationBins ? kStationBins - 1 : b);
        ++n[b];
        ++cover[b * kCoverBins / kStationBins];
        sum[b]  += r;
        sum2[b] += r * r;
    }
    float   excess = 0.0f;
    uint8_t used   = 0;
    uint8_t empty  = 0;
    for (uint8_t b = 0; b < kCoverBins; ++b) {
        if (cover[b] < mCfg.minBinPoints) {
            ++empty;
        }
    }
    for (uint8_t b = 0; b < kStationBins; ++b) {
        if (n[b] < 2) {
            continue;
        }
        const float nb   = static_cast<float>(n[b]);
        const float mean = sum[b] / nb;
        float var = (sum2[b] - nb * mean * mean) / (nb - 1.0f);
        if (var < 0.0f) {
            var = 0.0f;
        }
        excess += mean * mean - var / nb;
        ++used;
    }
    info.shapeM    = (used > 0 && excess > 0.0f) ? std::sqrt(excess / used) : 0.0f;
    info.emptyBins = empty;

    const float rLo = mCfg.fitRadiusMinM;
    const float rHi = mCfg.fitRadiusMaxM;
    if (empty > 0) {
        info.failed = Reject::Coverage;
    } else if (p.kerbR <= rLo + kEdgeM || p.kerbR >= rHi - kEdgeM) {
        info.failed = Reject::RadiusEdge;
    } else if (scale > mCfg.maxScaleM) {
        info.failed = Reject::Residual;
    } else if (info.shapeM > mCfg.maxShapeM) {
        info.failed = Reject::Shape;
    } else {
        info.failed = Reject::None;
    }
}

Calibrator::Change Calibrator::runFit() noexcept
{
    Params p;
    if (mHavePrev) {
        p = mPrev;
    } else {
        // Start from the centroid and the main axis of the points.
        double sx = 0.0, sy = 0.0;
        for (uint16_t i = 0; i < mCount; ++i) {
            const Vec2 q = pointAt(i);
            sx += q.x;
            sy += q.y;
        }
        const double mx = sx / mCount, my = sy / mCount;
        double sxx = 0.0, syy = 0.0, sxy = 0.0;
        for (uint16_t i = 0; i < mCount; ++i) {
            const Vec2   q  = pointAt(i);
            const double dx = q.x - mx, dy = q.y - my;
            sxx += dx * dx;
            syy += dy * dy;
            sxy += dx * dy;
        }
        p.cx      = static_cast<float>(mx);
        p.cy      = static_cast<float>(my);
        p.heading = static_cast<float>(0.5 * std::atan2(2.0 * sxy, sxx - syy));
        p.kerbR   = 36.5f;
        p.runner  = 0.0f;
    }

    FitInfo info;
    info.valid = true;
    info.laps  = mLaps;
    if (!fit(p, info.iterations)) {
        info.failed = Reject::Residual;
        mFit = info;
        if (mLaps >= mCfg.calRejectLaps) {
            mStage  = Stage::Rejected;
            mReject = Reject::Residual;
            return Change::Rejected;
        }
        return Change::None;
    }
    assess(p, info);

    bool agree = false;
    if (mHavePrev) {
        const float dx = p.cx - mPrev.cx, dy = p.cy - mPrev.cy;
        info.centreMoveM = std::sqrt(dx * dx + dy * dy);
        // Headings are the same half a turn apart.
        float dh = std::fmod(std::fabs(p.heading - mPrev.heading), kPi);
        if (dh > 0.5f * kPi) {
            dh = kPi - dh;
        }
        info.headingMoveDeg = dh * kRadDeg;
        agree = info.centreMoveM <= mCfg.lockCentreM && info.headingMoveDeg <= mCfg.lockHeadingDeg;
    }
    mFit = info;

    if (info.failed != Reject::None) {
        // A lap or two of noisy points can miss on a standard track; from
        // calRejectLaps on, a failed check means the track is not standard.
        if (mLaps >= mCfg.calRejectLaps) {
            mStage  = Stage::Rejected;
            mReject = info.failed;
            return Change::Rejected;
        }
        return Change::None;
    }

    mPrev     = p;
    mHavePrev = true;
    mModel.set({p.cx, p.cy}, p.heading, p.kerbR, mClockwise);
    mModel.setAltitudeM(mAltCount > 0 ? static_cast<float>(mAltSum / mAltCount)
                                      : std::numeric_limits<float>::quiet_NaN());

    if (mLaps >= mCfg.calLockLaps && agree) {
        mStage = Stage::Locked;
        return Change::Locked;
    }
    if (mLaps >= mCfg.calMaxLaps) {
        mStage  = Stage::Rejected;
        mReject = Reject::Disagree;
        mFit.failed = Reject::Disagree;
        return Change::Rejected;
    }
    if (mStage == Stage::Collecting) {
        mStage = Stage::Provisional;
        return Change::Provisional;
    }
    mStage = Stage::Refined;
    return Change::Refined;
}

} // namespace Oval
