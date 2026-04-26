/**
 * @file SimpleLPF.hpp
 * @date 07-02-2025
 * @author Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief Simple first-order low-pass filter (exponential moving average)
 */

#ifndef SDK_FILTER_SIMPLE_LPF_HPP
#define SDK_FILTER_SIMPLE_LPF_HPP

namespace SDK::Filter {

/**
 * @brief Simple first-order low-pass filter (exponential moving average)
 *
 * Output formula: y[n] = alpha * y[n-1] + (1 - alpha) * x[n]
 *
 * On first call to execute() after construction or reset(), the output equals
 * the input directly (no filtering on the initial sample to avoid startup transients).
 */
class SimpleLPF {
public:
    /**
     * @brief Construct with default smoothing factor (alpha = 0.5).
     */
    SimpleLPF()
        : mResetRequest(true)
        , mValue(0.0f)
        , mAlpha(0.5f)
    {}

    /**
     * @brief Construct with a specific smoothing factor.
     *
     * @param alpha Smoothing factor in range [0, 1).
     *              Values closer to 1 produce more smoothing (slower response).
     *              Values closer to 0 produce less smoothing (faster response).
     *              Silently clamped to 0.5 if outside valid range.
     */
    explicit SimpleLPF(float alpha)
        : mResetRequest(true)
        , mValue(0.0f)
        , mAlpha(0.5f)
    {
        setAlpha(alpha);
    }

    ~SimpleLPF() = default;

    /**
     * @brief Process a new input sample.
     *
     * @param input New input sample
     * @return Filtered output value
     */
    float execute(float input)
    {
        if (mResetRequest) {
            mResetRequest = false;
            mValue = input;
        } else {
            mValue = mValue * mAlpha + input * (1.0f - mAlpha);
        }
        return mValue;
    }

    /**
     * @brief Force the filter output to a specific value and reset history.
     *
     * @param value Value to force
     * @return The forced value
     */
    float forceValue(float value)
    {
        reset();
        return execute(value);
    }

    /**
     * @brief Get current filter output without processing a new sample.
     */
    float getValue() const { return mValue; }

    /**
     * @brief Reset filter state. Next execute() call will set output equal to input.
     */
    void reset() { mResetRequest = true; }

    /**
     * @brief Returns true if the filter has been reset and not yet processed a sample.
     */
    bool isReset() const { return mResetRequest; }

    /**
     * @brief Set smoothing factor. Ignored if value is outside [0, 1).
     *
     * @param alpha New smoothing factor in range [0, 1)
     */
    void setAlpha(float alpha)
    {
        if (alpha < 0.0f || alpha >= 1.0f) {
            return;
        }
        mAlpha = alpha;
    }

    /**
     * @brief Get current smoothing factor.
     */
    float getAlpha() const { return mAlpha; }

private:
    bool  mResetRequest; /* true until the first sample is processed after construction/reset */
    float mValue;        /* Current filter output */
    float mAlpha;        /* Smoothing factor in range [0, 1) */
};

}  // namespace SDK::Filter

#endif // SDK_FILTER_SIMPLE_LPF_HPP
