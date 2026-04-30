#ifndef __SIMPLE_HPF_HPP
#define __SIMPLE_HPF_HPP

#include <cstdint>
#include <cstdbool>
#include <functional>
#include <tuple>
#include "SDK/Simulator/Components/Filter/SimpleLPF.hpp"

namespace Filter {

    class SimpleHPF {
    public:
        SimpleHPF()
            : mLpf()
            , mValue(0.0f)
        {}

        SimpleHPF(float alpha)
            : mLpf()
            , mValue(0.0f)
        {
            mLpf.setAlpha(alpha);
        }

        float execute(float input)
        {
            return (mValue = input - mLpf.execute(input));
        }

        float getValue() const
        {
            return mValue;
        }

        void reset()
        {
            mLpf.reset();
            mValue = 0.0f;
        }

        void setAlpha(float alpha)
        {
            mLpf.setAlpha(alpha);
        }

        float getAlpha() const
        {
            return mLpf.getAlpha();
        }

    private:
        Filter::SimpleLPF mLpf;
        float             mValue;
    };

}

#endif // SIMPLE_HPF_HPP
