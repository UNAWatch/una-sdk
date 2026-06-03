/**
 ******************************************************************************
 * @file    CadenceSpikeFilter.hpp
 * @brief   Causal median spike-filter for wrist running-cadence samples.
 *
 * The kernel's wrist RUNNING_CADENCE estimator occasionally emits a brief
 * (1-2 s) valid-but-too-low cadence reading. Because the Treadmill app derives
 * speed as cadence x stride, each such sample becomes a downward speed spike on
 * the live display and in the recorded FIT. Treadmill cadence physically cannot
 * drop ~20-30 spm for a second and recover, so these are clearly artefacts.
 *
 * This is an app-side fix only -- it does NOT touch the kernel estimator. It is
 * a small causal median over the most recent valid samples: an isolated low (or
 * high) sample is outvoted by its neighbours and replaced by the median, while a
 * sustained genuine change is tracked with at most (window/2) samples of lag.
 *
 * Apply ONLY to samples the kernel flagged valid. Invalid samples (full
 * dropouts) must bypass this filter so the estimator's hold-forward still owns
 * dropout handling.
 *
 * Header-only (like SDK::Metric::VariableCounter) so it needs no build-manifest
 * changes.
 ******************************************************************************
 */

#ifndef __CADENCE_SPIKE_FILTER_HPP
#define __CADENCE_SPIKE_FILTER_HPP

#include <cstddef>

namespace SDK::Calibration
{

/**
 * @brief Causal median filter over recent valid cadence samples.
 *
 * Window is odd-friendly but any size in [1, kMaxWindow] works. A window of 1
 * is a pass-through. The default of 5 removes the dominant 1-2 s glitches with
 * ~2 s of lag at genuine cadence steps (validated on field data).
 */
class CadenceSpikeFilter
{
public:
    static constexpr std::size_t kMaxWindow     = 9;
    static constexpr std::size_t kDefaultWindow = 5;

    explicit CadenceSpikeFilter(std::size_t window = kDefaultWindow)
    {
        setWindow(window);
        reset();
    }

    /// Clear the sample history (call at session start). Window is preserved.
    void reset()
    {
        mCount = 0;
        mHead  = 0;
    }

    /// Change the window, clamped to [1, kMaxWindow]. Does not clear history.
    void setWindow(std::size_t window)
    {
        if (window < 1) {
            window = 1;
        } else if (window > kMaxWindow) {
            window = kMaxWindow;
        }
        mWindow = window;
    }

    std::size_t window() const { return mWindow; }

    /**
     * @brief Push one VALID cadence sample and return the de-spiked value.
     *
     * Pushes @p cadenceSpm into the rolling window and returns the median of the
     * samples currently held (1..window). The caller must only pass samples the
     * kernel flagged valid; invalid samples should bypass this method entirely.
     *
     * @param cadenceSpm Valid cadence sample, steps/min.
     * @return Median of the recent window, steps/min.
     */
    float filter(float cadenceSpm)
    {
        // Insert into the ring buffer.
        mBuf[mHead] = cadenceSpm;
        mHead = (mHead + 1) % mWindow;
        if (mCount < mWindow) {
            ++mCount;
        }

        // Copy the live samples and partial-sort for the median (window <= 9).
        float tmp[kMaxWindow];
        for (std::size_t i = 0; i < mCount; ++i) {
            tmp[i] = mBuf[i];
        }
        // Insertion sort: tiny, branch-predictable, no allocation.
        for (std::size_t i = 1; i < mCount; ++i) {
            float key = tmp[i];
            std::size_t j = i;
            while (j > 0 && tmp[j - 1] > key) {
                tmp[j] = tmp[j - 1];
                --j;
            }
            tmp[j] = key;
        }
        return tmp[mCount / 2];
    }

private:
    std::size_t mWindow = kDefaultWindow;
    std::size_t mCount  = 0;            ///< Valid samples currently held (<= mWindow).
    std::size_t mHead   = 0;            ///< Next write index into the ring.
    float       mBuf[kMaxWindow] = {};  ///< Ring buffer of recent valid samples.
};

} // namespace SDK::Calibration

#endif /* __CADENCE_SPIKE_FILTER_HPP */
