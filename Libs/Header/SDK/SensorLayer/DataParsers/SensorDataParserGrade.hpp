/**
 ******************************************************************************
 * @file    SensorDataParserGrade.hpp
 * @brief   Parser for GRADE sensor samples (barometric terrain grade).
 *
 * GRADE_PCT is only a
 * reliable estimate when GRADE_VALID is set.
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_GRADE_HPP
#define __SENSOR_DATA_PARSER_GRADE_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>

namespace SDK::SensorDataParser
{

class Grade
{
public:
    enum Field : uint8_t {
        GRADE_PCT   = 0,  ///< Terrain grade in percent; valid only when GRADE_VALID.
        GRADE_VALID = 1,  ///< 1 when grade_pct is a reliable current estimate.
        COUNT
    };

    explicit Grade(const SDK::Sensor::DataView data) : mData(data) {}

    bool isDataValid() const
    {
        return mData.getFieldCount() == Field::COUNT;
    }

    float getGradePct() const
    {
        return isDataValid() ? mData.f[Field::GRADE_PCT] : 0.0f;
    }

    bool isGradeValid() const
    {
        return isDataValid() && (mData.u[Field::GRADE_VALID] != 0);
    }

    uint32_t getTimestamp() const
    {
        return isDataValid() ? mData.getTimestamp() : 0;
    }

    static constexpr uint8_t getFieldsNumber()
    {
        return Field::COUNT;
    }

private:
    const SDK::Sensor::DataView mData;
};

} // namespace SDK::SensorDataParser

#endif /* __SENSOR_DATA_PARSER_GRADE_HPP */
