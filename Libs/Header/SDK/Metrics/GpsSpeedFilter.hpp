/**
 * @file GpsSpeedFilter.hpp
 * @brief Complementary filter: GPS Doppler speed for response, GPS position for scale
 */

#ifndef SDK_METRICS_GPS_SPEED_FILTER_HPP
#define SDK_METRICS_GPS_SPEED_FILTER_HPP

#include <cmath>
#include <cstddef>

namespace SDK::Metric {

/**
 * @brief Corrects the receiver's Doppler speed against the distance its own
 *        position track covers, and smooths it for display.
 *
 * The AG3335's reported speed over ground reads low while running, by an amount
 * that varies with the environment: measured against a reference watch over
 * eight simultaneous recordings it ran 0.7% low on an open road race and 5.8%
 * low under tree cover. The same receiver's POSITION track is sound throughout --
 * its summed path matched the odometer to within 1.2% on every one of those
 * runs, and matched the reference watch's position track to 0.1%. So the
 * information needed to correct the speed is already on the watch.
 *
 * The two channels are combined by frequency, which is what makes this cheap:
 *
 *   - the Doppler supplies the SHAPE. It responds quickly and is quiet
 *     second-to-second, but its scale drifts with signal conditions.
 *   - the position track supplies the SCALE. It is far too noisy per second to
 *     display, but over a few minutes the distance it covers is accurate.
 *
 * So a slowly-varying factor k = (distance from position) / (distance from
 * Doppler) is measured by exponentially averaging each distance with the time
 * constant @p ScaleTauTicks, and multiplied into the smoothed Doppler. Because k moves over minutes, it is effectively constant across a
 * change of pace, and the filter's step response is that of the Doppler
 * smoothing alone -- correcting the scale costs no responsiveness.
 *
 * Details that are easy to get wrong and are load-bearing here:
 *
 *   - **k is measured against the raw Doppler, never against this filter's own
 *     output.** Feeding the corrected speed back in would drive k to 1.0 and
 *     the correction would silently disappear.
 *   - **The position distance is a chord across @p ChordTicks seconds.** A chord
 *     is shorter than the path on every bend, so a long one reads k low; a very
 *     short one adds position noise instead. Over those eight runs the corrected
 *     speed averaged 0.997 of the reference with a 1 s chord, 0.994 with 2 s,
 *     and 0.989 with 5 s, while k wandered no more with the shorter chords.
 *
 * Measured over those runs against the reference, the speed moves from 0.942-
 * 0.993 of the reference uncorrected to 0.984-1.018 corrected, and the RMS
 * error across runs from 4.3% to 1.1%.
 *
 * @tparam SmoothTicks Doppler smoothing window, in ticks. Trades display
 *                     steadiness against responsiveness; it no longer affects
 *                     accuracy, because k absorbs the scale. Each extra tick
 *                     delays the readout by about half a second and steadies it
 *                     only a little, because the receiver's speed error is
 *                     correlated over tens of seconds. Measured on the raw
 *                     receiver speed over twenty 400 m rep transitions, against
 *                     a reference watch: at 8 ticks the readout reaches half a
 *                     change of pace within a second of the reference, with an
 *                     in-rep pace error of 9.6 s/km (sd); 5 ticks leads it by
 *                     1-2.5 s at 11.1 s/km, 10 ticks trails by up to 1.5 s at
 *                     8.9 s/km.
 * @tparam ScaleTauTicks Time constant of the exponential average that measures
 *                      k, in ticks. Long enough that k is steady across a change
 *                      of pace. Measured over six runs, shortening it buys
 *                      nothing: how far the correction wanders within a run is
 *                      flat from 20 s to 600 s, so a faster k only adds display
 *                      jitter (4.95 s/km at 20 s against 3.91 at 180 s). The
 *                      environment's effect on the receiver moves slowly.
 * @tparam ChordTicks  Chord length for the position distance, in ticks. Choose
 *                     it as a distance: at running pace 2 ticks is a 5-8 m
 *                     chord. At walking pace a longer one keeps the chord well
 *                     above the position noise.
 *
 * @note tick() is expected once per second, so the tick parameters are also
 *       seconds. Feed it the RAW latched values; it owns the smoothing.
 */
template <std::size_t SmoothTicks   = 8,
          std::size_t ScaleTauTicks = 180,
          std::size_t ChordTicks    = 2>
class GpsSpeedFilter {
    static_assert(SmoothTicks > 0, "GpsSpeedFilter needs a non-empty smoothing window");
    static_assert(ScaleTauTicks > 0, "GpsSpeedFilter needs a positive scale time constant");
    static_assert(ChordTicks  > 0, "GpsSpeedFilter needs a non-empty chord");

public:
    GpsSpeedFilter()
        : mMinValid(0.0f)
        , mMaxValid(0.0f)
        , mIsInitialized(false)
    {
        clearHistory();
        clearScale();
    }

    ~GpsSpeedFilter() = default;

    /**
     * @brief Initialize with the valid speed range.
     *
     * @param minValid Speed (m/s) below which no pace is reported. Samples below
     *                 it still enter the window -- slowing to a walk must pull
     *                 the readout down -- but a mean at or below it yields no pace.
     * @param maxValid Speed (m/s) above which a sample is discarded as bogus.
     * @return true on success, false if minValid >= maxValid
     */
    bool init(float minValid, float maxValid)
    {
        if (minValid >= maxValid) {
            return false;
        }
        mMinValid      = minValid;
        mMaxValid      = maxValid;
        mIsInitialized = true;
        clearHistory();
        clearScale();
        return true;
    }

    /**
     * @brief Discard everything, including the measured scale factor.
     *
     * For a track START. The scale returns to 1.0 and the filter behaves as
     * plain smoothing until it has re-measured, which takes a third of the time
     * constant in moving ticks.
     *
     * Do NOT use this for a pause -- see resetHistory().
     */
    void reset() { clearHistory(); clearScale(); }

    /**
     * @brief Discard the speed and position history but KEEP the scale factor.
     *
     * For a resume from a pause. The samples in hand describe an effort that is
     * over, but the scale factor does not describe the effort -- it describes how
     * far this receiver's speed is drifting in these conditions, which a stop
     * does not change. Discarding it would cost a third of the time constant in
     * moving ticks before the correction applied again, and the readout would
     * step by however much it had been out when it did. Under an auto-pause that
     * fires at every junction the filter could seldom leave that warm-up at all.
     */
    void resetHistory() { clearHistory(); }

    /**
     * @brief Advance one tick.
     *
     * Must be called every tick even when nothing is available, so that the
     * windows age: a lost fix empties them rather than being held forward.
     *
     * @param speedMs    Raw Doppler speed, m/s (ignored unless @p speedValid)
     * @param speedValid true when the speed came from a current, trustworthy fix
     * @param latDeg     Latitude in degrees  (ignored unless @p posValid)
     * @param lonDeg     Longitude in degrees (ignored unless @p posValid)
     * @param posValid   true when the position is from a current fix
     */
    void tick(float speedMs, bool speedValid, float latDeg, float lonDeg, bool posValid)
    {
        if (!mIsInitialized) {
            return;
        }

        const bool speedUsable = speedValid && speedMs >= 0.0f && speedMs <= mMaxValid;

        // --- Doppler smoothing -------------------------------------------------
        mSpeed[mHead]    = speedUsable ? speedMs : 0.0f;
        mSpeedOk[mHead]  = speedUsable;
        mPosLat[mHead]   = latDeg;
        mPosLon[mHead]   = lonDeg;
        mPosOk[mHead]    = posValid;

        /* Advance before deriving anything, so that "newest" means the same slot
           here as it does to every const accessor called after tick() returns. */
        mHead = (mHead + 1) % kRing;
        if (mFilled < kRing) {
            mFilled++;
        }

        const float smoothed = meanSpeed();

        // --- position chord ----------------------------------------------------
        // Compared across ChordTicks so that per-second position noise, which
        // inflates a summed path, averages out before it reaches k.
        float chordSpeed = 0.0f;
        bool  chordOk    = false;
        if (mFilled >= ChordTicks + 1) {
            const std::size_t now = newest();
            const std::size_t old = (now + kRing - ChordTicks) % kRing;
            if (mPosOk[now] && mPosOk[old]) {
                chordSpeed = separation(mPosLat[old], mPosLon[old],
                                        mPosLat[now], mPosLon[now])
                             / static_cast<float>(ChordTicks);
                chordOk = (chordSpeed <= mMaxValid);
            }
        }

        // --- scale factor ------------------------------------------------------
        // Both sides must cover the same seconds, so a tick only contributes when
        // the chord AND the smoothed Doppler are both available. The Doppler used
        // here is the smoothed RAW signal, never this filter's corrected output:
        // feeding the output back would drive k to 1.0 and cancel the correction.
        //
        // Ticks at or below the floor are excluded rather than merely non-zero.
        // Standing still, the Doppler correctly reads ~0 while the position chord
        // still reads GPS wander, so such a tick adds far more to the numerator
        // than to the denominator and walks k upward. Waiting at a crossing must
        // not teach the filter that the receiver reads low.
        if (chordOk && smoothed > mMinValid) {
            const float a = alpha();
            mScaleNum = mScaleNum * a + chordSpeed * (1.0f - a);
            mScaleDen = mScaleDen * a + smoothed   * (1.0f - a);
            if (mScaleCount < kScaleWarmup) {
                mScaleCount++;
            }
            updateScale();
        }
    }

    /**
     * @brief Smoothed, scale-corrected speed in m/s. 0.0 when nothing is known.
     *
     * This is the value to display, and to record as the per-record speed.
     */
    float getSpeed() const { return meanSpeed() * mScale; }

    /**
     * @brief Latest scale-corrected speed in m/s, without the smoothing.
     *
     * Use where a peak matters -- a session or lap maximum -- so that the
     * correction applies without the smoothing flattening the peak.
     */
    float getInstantSpeed() const
    {
        if (mFilled == 0 || !mSpeedOk[newest()]) {
            return 0.0f;
        }
        return mSpeed[newest()] * mScale;
    }

    /**
     * @brief Pace in s/m from getSpeed(), or 0.0 when none can be reported
     *        (nothing known, or at or below the minValid floor).
     */
    float getPace() const
    {
        const float v = getSpeed();
        return (v > mMinValid) ? (1.0f / v) : 0.0f;
    }

    /**
     * @brief The scale factor currently applied. 1.0 until it has been measured.
     *
     * Exposed for logging: it is the most direct evidence of how far the
     * receiver's speed is drifting, and how the environment is affecting it.
     */
    float getScale() const { return mScale; }

    /** @brief Number of Doppler samples in the smoothing window. */
    std::size_t getSampleCount() const
    {
        std::size_t n = 0;
        const std::size_t span = (mFilled < SmoothTicks) ? mFilled : SmoothTicks;
        for (std::size_t i = 0; i < span; i++) {
            if (mSpeedOk[(newest() + kRing - i) % kRing]) {
                n++;
            }
        }
        return n;
    }

    /** @brief Ticks that have contributed to the scale factor, up to the warm-up. */
    std::size_t getScaleSampleCount() const { return mScaleCount; }

    /**
     * @brief True while the smoothing window holds at least one sample.
     *
     * Still true for up to SmoothTicks - 1 ticks after the fix is lost, while
     * the window drains: right for the display, not for anything that needs a
     * current measurement -- see hasCurrentSample().
     */
    bool isValid() const { return getSampleCount() > 0; }

    /**
     * @brief True when the newest tick carried a usable speed.
     *
     * The gate for getInstantSpeed() and for anything recorded or calibrated
     * per tick: it goes false on the first tick without a current fix.
     */
    bool hasCurrentSample() const { return mFilled > 0 && mSpeedOk[newest()]; }

    /** @brief True once the scale factor has been measured from position. */
    bool isScaleMeasured() const { return mScaleMeasured; }

    static constexpr std::size_t getSmoothTicks() { return SmoothTicks; }
    static constexpr std::size_t getScaleTauTicks() { return ScaleTauTicks; }
    static constexpr std::size_t getChordTicks()  { return ChordTicks; }

    /** @brief Lower bound applied to the scale factor. */
    static constexpr float getScaleMin() { return 0.85f; }
    /** @brief Upper bound applied to the scale factor. */
    static constexpr float getScaleMax() { return 1.20f; }

    float getMinValid() const { return mMinValid; }
    float getMaxValid() const { return mMaxValid; }

private:
    /** @brief Index of the most recently written slot. Valid after tick() advances. */
    std::size_t newest() const { return (mHead + kRing - 1) % kRing; }

    /* The position ring must hold the chord as well as the smoothing window. */
    static constexpr std::size_t kRing =
        (SmoothTicks > ChordTicks + 1) ? SmoothTicks : (ChordTicks + 1);

    /* Contributing ticks before k is trusted at all: a third of the time
       constant, by which point the average has largely forgotten its seed. */
    static constexpr std::size_t kScaleWarmup =
        (ScaleTauTicks / 3) > 0 ? (ScaleTauTicks / 3) : 1;

    /** @brief Exponential decay per tick for the chosen time constant. */
    static float alpha()
    {
        static const float a = std::exp(-1.0f / static_cast<float>(ScaleTauTicks));
        return a;
    }

    void clearHistory()
    {
        for (std::size_t i = 0; i < kRing; i++) {
            mSpeed[i] = 0.0f; mSpeedOk[i] = false;
            mPosLat[i] = 0.0f; mPosLon[i] = 0.0f; mPosOk[i] = false;
        }
        mHead = 0; mFilled = 0;
    }

    void clearScale()
    {
        mScaleNum = 0.0f; mScaleDen = 0.0f; mScaleCount = 0;
        mScale = 1.0f;
        mScaleMeasured = false;
    }

    float meanSpeed() const
    {
        /* Summed on demand: at one tick per second the cost is irrelevant and it
           cannot accumulate drift. Only the newest SmoothTicks slots count, so a
           ring sized for the chord does not widen the smoothing. */
        float sum = 0.0f;
        std::size_t n = 0;
        const std::size_t span = (mFilled < SmoothTicks) ? mFilled : SmoothTicks;
        for (std::size_t i = 0; i < span; i++) {
            const std::size_t idx = (newest() + kRing - i) % kRing;
            if (mSpeedOk[idx]) {
                sum += mSpeed[idx];
                n++;
            }
        }
        return (n > 0) ? (sum / static_cast<float>(n)) : 0.0f;
    }

    void updateScale()
    {
        if (mScaleCount < kScaleWarmup || mScaleDen <= 0.0f) {
            return;     /* not enough evidence yet: hold the last factor */
        }
        float k = mScaleNum / mScaleDen;
        if (k < getScaleMin()) { k = getScaleMin(); }
        if (k > getScaleMax()) { k = getScaleMax(); }
        mScale         = k;
        mScaleMeasured = true;
    }

    /** @brief Metres between two positions; equirectangular, ample at these spans. */
    static float separation(float lat1, float lon1, float lat2, float lon2)
    {
        constexpr float kEarthR = 6371000.0f;
        constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;
        const float midLat = (lat1 + lat2) * 0.5f * kDegToRad;
        const float dx = (lon2 - lon1) * kDegToRad * std::cos(midLat) * kEarthR;
        const float dy = (lat2 - lat1) * kDegToRad * kEarthR;
        return std::sqrt(dx * dx + dy * dy);
    }

    float       mMinValid;
    float       mMaxValid;

    float       mSpeed[kRing];        /* raw Doppler samples */
    bool        mSpeedOk[kRing];
    float       mPosLat[kRing];
    float       mPosLon[kRing];
    bool        mPosOk[kRing];
    std::size_t mHead;
    std::size_t mFilled;

    /* Exponential averages of the two distances, rather than a ring of every
       tick: at these time constants a ring costs 1.6 kB per app and measures no
       better -- jitter tracks how fast k moves, not the shape of the average. */
    float       mScaleNum;      /* position distance, exponentially averaged */
    float       mScaleDen;      /* smoothed Doppler over the same ticks */
    std::size_t mScaleCount;    /* contributing ticks, saturating at the warm-up */

    float       mScale;
    bool        mScaleMeasured;
    bool        mIsInitialized;
};

}  // namespace SDK::Metric

#endif // SDK_METRICS_GPS_SPEED_FILTER_HPP
