/**
 ******************************************************************************
 * @file    SensorDataSample.cpp
 * @date    09-December-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Sensor data sample class with abbility to extend the cells count
 *          and access to the data fields.
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "SDK/Simulator/Components/SensorDataSample.hpp"

namespace Sensor {

float& DataSample::FloatWriter::operator[](uint16_t idx) const noexcept
{
    assert(idx < fieldCount);
    return data.mValue[idx].f;
}

uint32_t& DataSample::U32Writer::operator[](uint16_t idx) const noexcept
{
    assert(idx < fieldCount);
    return data.mValue[idx].u32;
}

int32_t& DataSample::I32Writer::operator[](uint16_t idx) const noexcept
{
    assert(idx < fieldCount);
    return data.mValue[idx].i32;
}

DataSample::DataSample(uint16_t fieldCount)
        : mData(createData(fieldCount))
        , mFieldCount(fieldCount)
        , f{*mData, fieldCount}
        , u{*mData, fieldCount}
        , i{*mData, fieldCount}
{
    assert(fieldCount > 0);
}

DataSample::~DataSample()
{
    ::operator delete(mData, std::align_val_t(4));
//    delete [] reinterpret_cast<uint8_t*>(mData);
}

SDK::Sensor::Data& DataSample::getData() noexcept
{
    return *mData;
}

const SDK::Sensor::Data& DataSample::getData() const noexcept
{
    return *mData;
}

std::size_t DataSample::getFieldCount() const noexcept
{
    return mFieldCount;
}

void DataSample::setTimestamp(uint32_t ms) const noexcept
{
    mData->mTimeStamp   = ms;
    mData->mTimeStampUs = 0;
}

void DataSample::setTimestampUs(uint64_t usTotal) const noexcept
{
    // mTimeStamp — ms, mTimeStampUs — "a tail" in us (0..999)
    mData->mTimeStamp   = static_cast<uint32_t>(usTotal / 1000ULL);
    mData->mTimeStampUs = static_cast<uint32_t>(usTotal % 1000ULL); // 0..999
}

SDK::Sensor::Data* DataSample::createData(std::size_t fieldCount)
{
    assert(fieldCount > 0);

    // Data has already contain one Cell (mValues[1]),
    // so add (fieldCount - 1) extra
    std::size_t bytes = sizeof(SDK::Sensor::Data) + (fieldCount - 1u) * sizeof(SDK::Sensor::Data::Field);

//    uint8_t* mem = new uint8_t[bytes];
    uint8_t* mem = (uint8_t*)::operator new(bytes, std::align_val_t(4)); // aligned

    if (!mem) {
        return nullptr;
    }

    std::memset(mem, 0, bytes);

    return reinterpret_cast<SDK::Sensor::Data*>(mem);
}

}
