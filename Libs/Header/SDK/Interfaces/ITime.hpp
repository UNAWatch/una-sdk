/**
 ******************************************************************************
 * @file    ITime.hpp
 * @date    10-07-2024
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Contains time-related interface definitions.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __INTERFACE_I_TIME_HPP
#define __INTERFACE_I_TIME_HPP

#include <cstdint>
#include <cstdbool>
#include <ctime>

namespace Interface
{

/**
 * @class ITime
 * @brief Interface for time management operations.
 */
class ITime {
public:

    /**
     * @brief   Time change callback
     */
    class Callback {
    public:

        /**
         * @brief   Called when the time changed more than 1 s.
         * @param   dateTime Structure containing the date and time.
         */
        virtual void onTimeChanged(const std::tm &dateTime) {}

    protected:

        /**
         * @brief   Destructor.
         */
        virtual ~Callback() = default;
    };

    /**
     * @brief Gets the current UTC time as time_t.
     * @return The current time.
     */
    virtual std::time_t time() = 0;

    /**
     * @brief Converts a UTC time structure to time_t.
     * @param utc_tm UTC time structure.
     * @return The corresponding time_t value.
     */
    virtual std::time_t timegm(std::tm &utc_tm) = 0;

    /**
     * @brief Converts a local time structure to time_t.
     * @param local_tm Local time structure.
     * @return The corresponding time_t value.
     */
    virtual std::time_t mktime(std::tm &local_tm) = 0;

    /**
     * @brief Gets the current local time.
     * @return Local time structure.
     */
    virtual std::tm localtime() = 0;

    /**
     * @brief Converts a UTC time_t value to local time structure.
     * @param utc UTC time value.
     * @return Local time structure.
     */
    virtual std::tm localtime(const std::time_t &utc) = 0;

    /**
     * @brief Converts a UTC time_t value to UTC time structure.
     * @param utc UTC time value.
     * @return UTC time structure.
     */
    virtual std::tm gmtime(const std::time_t &utc) = 0;

    /**
     * @brief Computes the difference in seconds between two time values.
     * @param time_end: The later time.
     * @param time_beg: The earlier time.
     * @return Difference in seconds.
     */
    virtual double difftime(std::time_t time_end, std::time_t time_beg) = 0;

    /**
     * @brief Formats a time structure into a string.
     * @param s Output buffer.
     * @param max Maximum buffer size.
     * @param format Format string.
     * @param tm Time structure.
     * @return Number of characters written (excluding null terminator).
     */
    virtual std::size_t strftime(char *s, size_t max, const char *format,
            const std::tm &tm) = 0;

    /**
     * @brief Gets the current timezone offset.
     * @return Timezone offset in seconds.
     */
    virtual int timezone() = 0;

    /**
     * @brief Gets the daylight saving time offset.
     * @return DST offset in seconds.
     */
    virtual int dstOffset() = 0;

    /**
     * @brief Sets the timezone offset.
     * @param tz Timezone offset in seconds.
     */
    virtual void setTimezone(int tz) = 0;

    /**
     * @brief Sets the daylight saving time offset.
     * @param dstoff DST offset in seconds.
     */
    virtual void setDstOffset(int dstoff) = 0;

    /**
     * @brief Sets the local time.
     * @param local_tm Local time structure.
     * @return True if successful, false otherwise.
     */
    virtual bool setLocaltime(const std::tm &local_tm) = 0;

    /**
     * @brief Sets the current UTC time.
     * @param utc UTC time value.
     * @return True if successful, false otherwise.
     */
    virtual bool setTime(const std::time_t &utc) = 0;

protected:

    /**
     * @brief Virtual destructor.
     */
    virtual ~ITime() = default;

};

} /* namespace Interface */

#endif /* __INTERFACE_I_TIME_HPP */
