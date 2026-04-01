#ifndef __SIMPL_LPF_HPP
#define __SIMPL_LPF_HPP

#include <cstdint>
#include <cstdbool>
#include <functional>
#include <tuple>

namespace Filter {

class SimpleLPF {
public:
    SimpleLPF()
        : mResetRequest(true)
        , mValue(0.0f)
        , mAlpha(0.5f)
    {}

    SimpleLPF(float alpha)
        : mResetRequest(true)
        , mValue(0.0f)
        , mAlpha(0.5f)
    {
        setAlpha(alpha);
    }

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

    float forceValue(float value)
    {
        reset();

        return execute(value);
    }

    float getValue() const
    {
        return mValue;
    }

    void reset()
    {
        mResetRequest = true;
    }

    bool isReseted() const
    {
        return mResetRequest;
    }

    void setAlpha(float alpha)
    {
        if (alpha < 0.0f || alpha >= 1.0f) {
            return;
        }

        mAlpha = alpha;
    }

    float getAlpha() const
    {
        return mAlpha;
    }

private:
    bool  mResetRequest;
    float mValue;
    float mAlpha;
};

}

#endif // SIMPL_LPF_HPP
