#include <gui/containers/SideBarBig.hpp>
#include <touchgfx/Application.hpp>
#include <touchgfx/EasingEquations.hpp>

SideBarBig::SideBarBig()
{

}

void SideBarBig::initialize()
{
    SideBarBigBase::initialize();
}

void SideBarBig::setCount(uint16_t cnt)
{
    if (cnt == 0) {
        cnt = 1;
    }

    mCount = cnt;
    handle.setVisible(mCount > 1);
    handle.invalidate();

    setActiveId(0);
}

void SideBarBig::setActiveId(uint16_t p)
{
    if (mCount <= 1) {
        p = 0;
    }

    mPosition = p;
    float start = getStartAngle(p);

    handle.setArc(start, start + kHandleLen);
    handle.invalidate();
    rail.invalidate();

}

void SideBarBig::animateToId(int16_t p, int16_t animationSteps)
{
    if (mCount <= 1) {
        p = 0;
    }

    bool ovfDir = false;

    if (p < 0) { 
        p = mCount - 1;
        mAnimationOvf = true;
    } else if (p >= mCount) { 
        p = 0;
        mAnimationOvf = true;
        ovfDir = true;
    } else { 
        mAnimationOvf = false;
    }

    if (animationSteps <= 0) {
        if (mAnimationRunning) {
            Application::getInstance()->unregisterTimerWidget(this);
            mAnimationRunning = false;
        }
        setActiveId(p);
    } else {
        if (!mAnimationRunning) {
            Application::getInstance()->registerTimerWidget(this);
        } 
            
        mAnimationDuration = animationSteps;

        handle.getArcStart(mAnimationStartPos);
        mAnimationEndPos = getStartAngle(p);

        if (mAnimationOvf) {
            if (ovfDir) {
                mAnimationFadeEndPos = mAnimationStartPos - kHandleLen;
                mAnimationOvfStartPos = mAnimationEndPos + kHandleLen;
            } else { 
                mAnimationFadeEndPos = mAnimationStartPos + kHandleLen;
                mAnimationOvfStartPos = mAnimationEndPos - kHandleLen;
            }
        }

        mAnimationRunning = true;
        mPosition = p;
    }
}

uint16_t SideBarBig::getActiveId()
{
    return mPosition;
}
    

void SideBarBig::nextAnimationStep() {
    if (mAnimationRunning) {
        mAnimationCounter++;

        float s = 0;
        
        if (!mAnimationOvf) {
            s = mAnimationStartPos + (mAnimationCounter * ((mAnimationEndPos - mAnimationStartPos) / mAnimationDuration));
        } else { 
            s = mAnimationStartPos + (mAnimationCounter * ((mAnimationFadeEndPos - mAnimationStartPos) / mAnimationDuration));

            float sOvf = mAnimationOvfStartPos + (mAnimationCounter * ((mAnimationEndPos - mAnimationOvfStartPos) / mAnimationDuration));
            handleOvf.setArc(sOvf, sOvf + kHandleLen);
            handleOvf.setVisible(true);
            handleOvf.invalidate();
        }

        handle.setArc(s, s + kHandleLen);
        handle.invalidate();
        rail.invalidate();

        if (mAnimationCounter >= (uint32_t)(mAnimationDuration)) {
            // End of animation
            mAnimationRunning = false;
            mAnimationCounter = 0;
            Application::getInstance()->unregisterTimerWidget(this);

            if (mAnimationOvf) {
                mAnimationOvf = false;
                handleOvf.setVisible(false);
                handleOvf.invalidate();
                setActiveId(mPosition);
            }
        }
    }
}

float SideBarBig::getStartAngle(uint16_t p)
{
    float offset = (kHandleMax - kHandleMin) / 2;

    if (mCount > 1) {
        float step = (kHandleMax - kHandleMin - kHandleLen) / (mCount - 1);
        offset = step * p;
    }

    return (kHandleMax - kHandleLen) - offset;
}