#ifndef SCROLLLARGE_HPP
#define SCROLLLARGE_HPP

#include <gui_generated/containers/SideBarBigBase.hpp>

class SideBarBig : public SideBarBigBase
{
public:
    SideBarBig();
    virtual ~SideBarBig() {}

    virtual void initialize();

    void setCount(uint16_t cnt);
    void setActiveId(uint16_t p);
    void animateToId(int16_t p, int16_t animationSteps = -1);
    uint16_t getActiveId();
    virtual void handleTickEvent()
    {
        nextAnimationStep();
    }

protected:
    const float kHandleLen = 18;
    const float kHandleMin = 232;
    const float kHandleMax = 308;

    uint16_t mCount = 1;
    uint16_t mPosition = 0;

    bool mAnimationRunning { };
    uint16_t mAnimationCounter { };
    uint16_t mAnimationDuration { };
    float mAnimationStartPos { };
    float mAnimationEndPos { };

    float mAnimationFadeEndPos { };

    bool mAnimationOvf {};
    float mAnimationOvfStartPos { };

    void nextAnimationStep();
    float getStartAngle(uint16_t p);
};

#endif // SCROLLLARGE_HPP
