/*******************************************************************************
 * @file   WristTiltDetector.hpp
 * @date   22-04-2025
 * @author Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief  Wrist-tilt (watch-look) gesture detector for walking and running.
 *
 *         Algorithm overview:
 *           1. MOTION CLASSIFICATION  (5 s sliding window on AY delta)
 *              Computes avg_swing = mean(|ay[i] - ay[i-1]|) over the window.
 *              Maps avg_swing to one of three motion classes:
 *                STATIONARY, WALKING, RUNNING.
 *              Hysteresis: state only changes when a threshold is strictly
 *              crossed; the STATIONARY class has a dedicated lower bound so
 *              that noise near zero does not flip the state back and forth.
 *
 *           2. DYNAMIC GX THRESHOLD
 *              Each motion class has its own peak-GX detection threshold
 *              (raw int16 LSB from the sensor register).  Higher thresholds
 *              for more vigorous activities prevent running arm-swing from
 *              flooding the gesture output.
 *
 *           3. TILT STATE MACHINE
 *              IDLE   -> if |GX| exceeds the dynamic threshold -> ACTIVE
 *              ACTIVE -> held for holdDurationS seconds, listener notified
 *                        once on entry -> COOLDOWN
 *              COOLDOWN -> held for cooldownDurationS seconds -> IDLE
 *
 *         Raw LSB units (not converted to physical units) are used throughout
 *         for efficiency and to avoid floating-point on resource-limited MCUs.
 *
 *         Default tuning values are taken directly from wrist_tilt11.py and
 *         validated on three BMI270 datasets (running, walking, stationary):
 *           motionWindowS     = 5.0   s
 *           avgSwingStationary= 50    LSB
 *           avgSwingWalking   = 60    LSB
 *           avgSwingRunning   = 400   LSB
 *           gxThreshStationary= 3000  LSB
 *           gxThreshWalking   = 5100  LSB
 *           gxThreshRunning   = 5300  LSB
 *           holdDurationS     = 3.0   s
 *           cooldownDurationS = 0.5   s
 *
 *         Sample rate assumption: 100 Hz (10 ms / sample).
 *         Adjustable via Config::sampleRateHz.
 ******************************************************************************/

#ifndef WRIST_TILT_DETECTOR_HPP
#define WRIST_TILT_DETECTOR_HPP

#include <cstdint>
#include <cstddef>

 /*******************************************************************************
  * @brief Single IMU sample carrying the two axes used by this detector.
  *
  *        Values are raw 16-bit signed integers as delivered by the sensor
  *        FIFO — no scaling or unit conversion applied.
  ******************************************************************************/
struct TiltImuSample {
    int16_t  ayLsb; /**< Accelerometer Y axis, raw LSB (motion classification) */
    int16_t  gxLsb; /**< Gyroscope    X axis, raw LSB (gesture trigger)        */
    uint32_t timestampMs; /**< Absolute timestamp of this sample, ms           */
};

/*******************************************************************************
 * @brief Motion activity class reported by the internal classifier.
 ******************************************************************************/
enum class MotionClass : uint8_t {
    STATIONARY = 0,
    WALKING = 1,
    RUNNING = 2,
};

/*******************************************************************************
 * @brief Wrist-tilt gesture detector.
 *
 *        Thread safety: NOT thread-safe. Call addBatch() from a single task.
 *
 *        Typical usage (FIFO interrupt context, 50-sample batches at 100 Hz):
 *
 *          WristTiltDetector detector;
 *          detector.setListener(&myListener);
 *
 *          // in FIFO callback:
 *          TiltImuSample batch[50];
 *          fillBatchFromFifo(batch, 50);
 *          detector.addBatch(batch, 50);
 ******************************************************************************/
class WristTiltDetector {
public:
    /*--------------------------------------------------------------------------
     * Configuration
     *------------------------------------------------------------------------*/
    struct Config {
        /** Input sample rate, Hz.  Used to size the motion window and timers. */
        float sampleRateHz = 100.0f;

        /**
         * Motion classification window length, seconds.
         * avg_swing is computed over the most recent (sampleRateHz * motionWindowS)
         * samples.  Longer windows give more stable classification but respond
         * slower to activity changes.
         */
        float motionWindowS = 5.0f;

        /**
         * @defgroup MotionThresholds  avg_swing thresholds (raw LSB)
         *
         * avg_swing = mean of |ay[i] - ay[i-1]| over the motion window.
         *
         * Classification logic:
         *   avg_swing > avgSwingRunning  -> RUNNING
         *   avg_swing > avgSwingWalking  -> WALKING
         *   avg_swing < avgSwingStationary -> STATIONARY
         *   (else: retain current class — hysteresis band between thresholds)
         * @{
         */
        float avgSwingStationary = 50.0f;  /**< Upper bound for STATIONARY   */
        float avgSwingWalking = 60.0f;  /**< Lower bound for WALKING      */
        float avgSwingRunning = 400.0f; /**< Lower bound for RUNNING      */
        /** @} */

        /**
         * @defgroup GxThresholds  Peak |GX| thresholds per motion class (raw LSB)
         *
         * A single sample must exceed this threshold to trigger the state
         * machine transition IDLE -> ACTIVE.
         * @{
         */
        int32_t gxThreshStationary = 3000; /**< LSB — stationary wrist tilt  */
        int32_t gxThreshWalking = 5100; /**< LSB — walking  wrist tilt    */
        int32_t gxThreshRunning = 5300; /**< LSB — running  wrist tilt    */
        /** @} */

        /**
         * Duration to hold the ACTIVE state after a gesture is detected, s.
         * The listener is called once when entering ACTIVE.
         */
        float holdDurationS = 3.0f;

        /** Minimum gap between two consecutive gesture events, s. */
        float cooldownDurationS = 0.5f;
    };

    /*--------------------------------------------------------------------------
     * IListener
     *------------------------------------------------------------------------*/
    class IListener {
    public:
        virtual ~IListener() = default;

        /**
         * @brief Called once when a wrist-tilt gesture is detected.
         * @param timestampMs Timestamp of the triggering sample, ms.
         */
        virtual void onWristTilt(uint32_t timestampMs) = 0;

        /**
         * @brief Called every time the detected motion class changes.
         * @param motionClass New motion class.
         * @param timestampMs Timestamp of the sample that caused the change, ms.
         */
        virtual void onMotionClassChange(MotionClass motionClass,
            uint32_t    timestampMs)
        {}
    };

    /*--------------------------------------------------------------------------
     * Construction / lifecycle
     *------------------------------------------------------------------------*/

     /**
      * @brief Construct with explicit configuration.
      * @param cfg Tuning parameters.
      */
    explicit WristTiltDetector(const Config& cfg) noexcept;

    /** @brief Construct with default configuration. */
    WristTiltDetector() noexcept : WristTiltDetector(Config{}) {}

    /**
     * @brief Set or replace the event listener.
     * @param listener Pointer to listener; nullptr disables all callbacks.
     */
    void setListener(IListener* listener) noexcept;

    /**
     * @brief Reset all internal state to initial values.
     *        Configuration and listener are preserved.
     */
    void reset() noexcept;

    /**
     * @brief Apply a new configuration and reset all internal state.
     *
     *        Equivalent to constructing a fresh instance with the given config
     *        while keeping the current listener.  Must be called before the
     *        first addBatch() call when the default constructor was used and
     *        non-default tuning is required.
     *
     * @param cfg New tuning parameters to apply.
     */
    void setConfig(const Config& cfg) noexcept;

    /*--------------------------------------------------------------------------
     * Main processing
     *------------------------------------------------------------------------*/

     /**
      * @brief Process one batch of IMU samples.
      *
      *        Must be called for every incoming batch in chronological order.
      *        Samples within the batch must also be in chronological order.
      *
      * @param samples Pointer to array of TiltImuSample.
      * @param count   Number of valid samples in the array.
      * @return true  if at least one gesture event was fired in this batch.
      * @return false otherwise.
      */
    bool addBatch(const TiltImuSample* samples, size_t count) noexcept;

    /*--------------------------------------------------------------------------
     * Diagnostics
     *------------------------------------------------------------------------*/

     /** @brief Total gesture events fired since construction or last reset(). */
    uint32_t    totalEvents()       const noexcept { return mTotalEvents; }

    /** @brief Current motion classification. */
    MotionClass currentMotionClass() const noexcept { return mMotionClass; }

    /** @brief Current avg_swing value (last computed, raw LSB). */
    float       lastAvgSwing()      const noexcept { return mLastAvgSwing; }

private:
    /*--------------------------------------------------------------------------
     * Internal types
     *------------------------------------------------------------------------*/
    enum class TiltState : uint8_t { IDLE, ACTIVE, COOLDOWN };

    /*--------------------------------------------------------------------------
     * Internal helpers
     *------------------------------------------------------------------------*/

     /**
      * @brief Push one AY sample into the sliding window and update mGxSum.
      * @param ay    AY value of the new sample.
      * @param prevAy AY value of the previous sample (for delta computation).
      */
    void  pushMotionSample(int16_t ay, int16_t prevAy) noexcept;

    /**
     * @brief Compute current avg_swing from the running sum.
     * @retval Average absolute delta of AY over the motion window, LSB.
     */
    float computeAvgSwing() const noexcept;

    /**
     * @brief Update mMotionClass based on current avg_swing.
     * @param avgSwing  Result of computeAvgSwing().
     * @param tsMs      Timestamp of the triggering sample for the callback.
     */
    void  updateMotionClass(float avgSwing, uint32_t tsMs) noexcept;

    /**
     * @brief Return the GX detection threshold for the current motion class.
     * @retval Threshold in raw LSB.
     */
    int32_t dynamicGxThreshold() const noexcept;

    /**
     * @brief Process one sample through the tilt state machine.
     * @param sample  Current IMU sample.
     * @return true if a gesture event was fired.
     */
    bool processSample(const TiltImuSample& sample) noexcept;

    /*--------------------------------------------------------------------------
     * Motion window ring buffer
     *
     * Stores the absolute delta |ay[i] - ay[i-1]| for each sample so the
     * running sum can be maintained in O(1) per sample.
     *------------------------------------------------------------------------*/
    static constexpr uint16_t MAX_MOTION_WIN = 1024u; /**< Max window samples */

    float    mDeltaBuf[MAX_MOTION_WIN] = {}; /**< Ring: |ay[i] - ay[i-1]|    */
    uint16_t mMotionHead = 0u; /**< Write index in ring         */
    uint16_t mMotionFilled = 0u; /**< Samples stored (<= capacity)*/
    uint16_t mMotionCapacity = 0u; /**< Window size in samples      */
    float    mDeltaSum = 0.0f; /**< Running sum of mDeltaBuf  */

    int16_t  mPrevAy = 0;  /**< Previous AY for delta calc  */
    bool     mHasPrevAy = false;

    /*--------------------------------------------------------------------------
     * Tilt state machine
     *------------------------------------------------------------------------*/
    TiltState mTiltState = TiltState::IDLE;
    uint32_t  mStateDeadline = 0u; /**< Sample count when state timer expires */
    uint32_t  mSampleCount = 0u; /**< Total samples processed               */

    uint32_t  mHoldSamples = 0u; /**< holdDurationS    * sampleRateHz      */
    uint32_t  mCooldownSamples = 0u; /**< cooldownDurationS * sampleRateHz     */

    /*--------------------------------------------------------------------------
     * Classification & diagnostics
     *------------------------------------------------------------------------*/
    MotionClass mMotionClass = MotionClass::STATIONARY;
    float       mLastAvgSwing = 0.0f;
    uint32_t    mTotalEvents = 0u;

    Config      mCfg;
    IListener* mListener = nullptr;
};

#endif // WRIST_TILT_DETECTOR_HPP
