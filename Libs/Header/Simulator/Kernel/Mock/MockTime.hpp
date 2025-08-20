/**
 ******************************************************************************
 * @file    MockTime.hpp
 * @date    04-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Mock for ITime interface.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SIMULATOR_KERNEL_TIME_HPP
#define __SIMULATOR_KERNEL_TIME_HPP

#include "API/Time.hpp"
#include "Wrappers/StdLibWrappers.h"

#include <cstdint>
#include <windows.h>
#include <chrono>
#include <ctime>

namespace Simulator
{

/**
 * @class MockTime
 * @brief A mock implementation of a time class that takes into account
 *        user-defined time offsets.
 *
 * This class is a virtual implementation of the @ref Interface::ITime
 * interface, which allows you to manipulate time using the system time
 * with the ability to take into account user-defined changes in the form
 * of time zone offsets and daylight saving time.
 * By default, the time is system time, but using the class methods, the
 * user can set their own offsets relative to the system time, such as
 * time zone offsets and daylight saving time offsets.
 */
class MockTime : public sdk::api::Time {
public:
    MockTime() 
        : mTimeZoneOffset(0), mDstOffset(0)
    {
        // Set timezone and DST same as in system
        std::time_t now   = std::time(nullptr);
        std::tm     local = safe_localtime(&now);
        std::tm     utc   = safe_gmtime(&now);

        utc.tm_isdst = local.tm_isdst; // mktime should has same dst for correct conversion
        int offset = static_cast<int>(std::difftime(std::mktime(&local), std::mktime(&utc)));

        if (local.tm_isdst > 0) {
            mDstOffset = 3600;
        }
        mTimeZoneOffset = offset - mDstOffset;
    }

    virtual ~MockTime() = default;

    virtual std::time_t time() override
    {
        return std::time(nullptr) + mUserOffset;
    }

    virtual std::time_t timegm(std::tm &utc_tm) override
    {
        std::time_t now   = std::time(nullptr);
        std::tm     local = safe_localtime(&now);
        std::tm     utc   = safe_gmtime(&now);

        utc.tm_isdst = local.tm_isdst; // mktime should has same dst for correct conversion
        int offset = static_cast<int>(std::difftime(std::mktime(&local), std::mktime(&utc)));

        // We should use DST same as in locale for correct conversion
        std::tm utc_tm_copy = utc_tm;
        utc_tm_copy.tm_isdst = local.tm_isdst;
        return std::mktime(&utc_tm_copy) + offset;
    }

    virtual std::time_t mktime(std::tm &local_tm) override
    {
        return this->timegm(local_tm) - mTimeZoneOffset - mDstOffset;
    }

    virtual std::tm localtime()
    {
        std::time_t virt = time();
        virt += mTimeZoneOffset + mDstOffset;

        std::tm t  = safe_gmtime(&virt);
        t.tm_isdst = mDstOffset > 0 ? 1 : 0;

        return t;
    }

    virtual std::tm localtime(const std::time_t &utc) override
    {
        std::time_t virt = utc + mTimeZoneOffset + mDstOffset;

        std::tm t  = safe_gmtime(&virt);
        t.tm_isdst = mDstOffset > 0 ? 1 : 0;

        return t;
    }

    virtual std::tm gmtime(const std::time_t &utc) override
    {
        return safe_gmtime(&utc);
    }

    virtual double difftime(std::time_t time_end, std::time_t time_beg) override
    {
        return std::difftime(time_end, time_beg);
    }

    virtual std::size_t strftime(char *s, size_t max, const char *format,
        const std::tm &tm) override
    {
        return std::strftime(s, max, format, &tm);
    }

    virtual int timezone() override
    {
        return mTimeZoneOffset;
    }

    virtual int dstOffset() override
    {
        return mDstOffset;
    }

    virtual void setTimezone(int tz) override
    {
        mTimeZoneOffset = tz;
    }

    virtual void setDstOffset(int dstoff) override
    {
        mDstOffset = dstoff;
    }

    virtual bool setLocaltime(const std::tm &local_tm) override 
    { 
        std::tm local = local_tm;   // make a modifiable copy
        std::time_t user_utc = this->mktime(local);

        return setTime(user_utc);
    }

    virtual bool setTime(const std::time_t &utc) override
    {
        std::time_t current = std::time(nullptr);
        mUserOffset = static_cast<int>(std::difftime(utc, current));
        return true;
    }

private:
    std::time_t mUserOffset = 0; // user offset relative to real UTC
    int mTimeZoneOffset = 0; // timezone offset (in seconds)
    int mDstOffset = 0; // DST offset (in seconds)

};

} /* namespace Simulator */

#endif /* __SIMULATOR_KERNEL_TIME_HPP */