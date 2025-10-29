/**
 ******************************************************************************
 * @file    FitHelper.hpp
 * @date    28-October-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   FIT message helper class
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "SDK/FitHelper/FitHelper.hpp"

#include <unordered_set>

namespace SDK::Component {

FitHelper::FitHelper(uint8_t             msgID, 
                     const FIT_MESG_DEF& msgDef,
                     uint16_t            msgDefSize)
	: mMsgID(msgID)
    , mMsgDefOrigin(msgDef)
	, mMsgDefOriginSize(msgDefSize)
    , mMsgDef()
    , mMsgDefFields()
	, mDataCRC(0)
{
}

bool FitHelper::init(std::initializer_list<FIT_EVENT_FIELD_NUM> fields)
{
    if (!isUnique(fields)) {
        return false;
    }

    if (!isValid(fields)) {
        return false;
    }

	makeMsgDef(fields);

    makeMsgFields(fields);

    return true;
}

bool FitHelper::writeDef(SDK::Interface::IFile* fp)
{
    WriteMessageDefinition(mMsgID,
                           (const void*)&mMsgDef,
                           static_cast<FIT_UINT16>(sizeof(FIT_MESG_DEF) - FIT_FLEX_ARRAY + mMsgDef.num_fields * FIT_FIELD_DEF_SIZE),
						   fp);
    return true;
}

bool FitHelper::writeData(void* data, SDK::Interface::IFile * fp)
{
    //Fit_InitMesg((const FIT_MESG_DEF*)&mMsgDef, data);

    WriteData(&mMsgID, FIT_HDR_SIZE, fp);

    const uint8_t* buff = (const uint8_t*) data;

    for (uint8_t idx = 0; idx < mMsgDef.num_fields; ++idx) {
        WriteData(&buff[mMsgFields[idx].offset], mMsgFields[idx].size, fp);
    }

	return true;
}

bool FitHelper::isUnique(std::initializer_list<FIT_EVENT_FIELD_NUM>& fields)
{
    std::unordered_set<FIT_EVENT_FIELD_NUM> uniq;
    uniq.reserve(fields.size());

    for (auto f : fields) {
        auto [it, inserted] = uniq.insert(f);
        if (!inserted) {
            return false;
        }
    }

    return true;
}

bool FitHelper::isValid(std::initializer_list<FIT_EVENT_FIELD_NUM>& fields)
{
    for (auto f : fields) {
		bool ok = false;
        for (uint8_t idx = 0; idx < mMsgDefOrigin.num_fields; ++idx) {
            if (f == mMsgDefOrigin.fields[idx * FIT_FIELD_DEF_SIZE]) {
                ok = true;
                break;
            }
        }

        if (!ok) {
            return false;
	    }
    }

    return true;
}

void FitHelper::makeMsgDef(std::initializer_list<FIT_EVENT_FIELD_NUM>& fields)
{
    mMsgDefFields  = std::make_unique<FIT_UINT8[]>(fields.size() * FIT_FIELD_DEF_SIZE);
    mMsgDef.fields = mMsgDefFields.get();

    mMsgDef.reserved_1      = mMsgDefOrigin.reserved_1;
    mMsgDef.arch            = mMsgDefOrigin.arch;
    mMsgDef.global_mesg_num = mMsgDefOrigin.global_mesg_num;
    mMsgDef.num_fields      = static_cast<FIT_UINT8>(fields.size());

	uint8_t offset = 0;

    for (auto f : fields) {
        for (uint8_t idx = 0; idx < mMsgDefOrigin.num_fields; ++idx) {
            if (f == mMsgDefOrigin.fields[idx * FIT_FIELD_DEF_SIZE]) {
                memcpy(&mMsgDef.fields[offset], &mMsgDefOrigin.fields[idx * FIT_FIELD_DEF_SIZE], FIT_FIELD_DEF_SIZE);
                offset += FIT_FIELD_DEF_SIZE;
                break;
            }
        }
    }
}

void FitHelper::makeMsgFields(std::initializer_list<FIT_EVENT_FIELD_NUM>& fields)
{
    mMsgFields = std::make_unique<MsgField[]>(fields.size());
    uint16_t idx = 0;
    for (auto f : fields) {
        mMsgFields[idx].offset = getFieldOffset(f);
        mMsgFields[idx].size   = getFieldSize(f);
        ++idx;
	}
}

uint16_t FitHelper::getFieldOffset(FIT_EVENT_FIELD_NUM field)
{
    uint16_t offset = 0;
    for (FIT_UINT8 idx = 0; idx < mMsgDefOrigin.num_fields; ++idx) {
        if (field == mMsgDefOrigin.fields[FIT_MESG_DEF_FIELD_OFFSET(field_def_num, idx)]) {
            return offset;
        }
        
        FIT_UINT8 base_type_num = mMsgDefOrigin.fields[FIT_MESG_DEF_FIELD_OFFSET(base_type, idx)] & FIT_BASE_TYPE_NUM_MASK;
        if (base_type_num >= FIT_BASE_TYPES) {
            return UINT16_MAX;
        }
        
        FIT_UINT8 base_type_size = fit_base_type_sizes[base_type_num];
        FIT_UINT8 fieldSize      = mMsgDefOrigin.fields[FIT_MESG_DEF_FIELD_OFFSET(size, idx)];
        offset += fieldSize * base_type_size;
    }
	
    return UINT16_MAX;
}

uint16_t FitHelper::getFieldSize(FIT_EVENT_FIELD_NUM field)
{
    for (FIT_UINT8 idx = 0; idx < mMsgDefOrigin.num_fields; ++idx) {
        if (field == mMsgDefOrigin.fields[FIT_MESG_DEF_FIELD_OFFSET(field_def_num, idx)]) {
            FIT_UINT8 base_type_num = mMsgDefOrigin.fields[FIT_MESG_DEF_FIELD_OFFSET(base_type, idx)] & FIT_BASE_TYPE_NUM_MASK;
            if (base_type_num >= FIT_BASE_TYPES) {
                return UINT16_MAX;
            }

            FIT_UINT8 base_type_size = fit_base_type_sizes[base_type_num];
            FIT_UINT8 fieldSize      = mMsgDefOrigin.fields[FIT_MESG_DEF_FIELD_OFFSET(size, idx)];

            return fieldSize * base_type_size;
        }
    }

    return UINT16_MAX;
}

void FitHelper::WriteFileHeader(SDK::Interface::IFile* fp)
{
    FIT_FILE_HDR file_header{};

    file_header.header_size = FIT_FILE_HDR_SIZE;
    file_header.profile_version = FIT_PROFILE_VERSION;
    file_header.protocol_version = FIT_PROTOCOL_VERSION_20;
    memcpy((FIT_UINT8*)&file_header.data_type, ".FIT", 4);

    fp->flush();
    size_t fileSize = fp->size();

    if (fileSize > FIT_FILE_HDR_SIZE - sizeof(FIT_UINT16)) {
        file_header.data_size = static_cast<FIT_UINT32>(fileSize - FIT_FILE_HDR_SIZE - sizeof(FIT_UINT16));
    }
    else {
        file_header.data_size = 0;
    }

    file_header.crc = FitCRC_Calc16(&file_header, FIT_STRUCT_OFFSET(crc, FIT_FILE_HDR));

    fp->seek(0);

    size_t bw;
    fp->write(reinterpret_cast<const char*>(&file_header), FIT_FILE_HDR_SIZE, bw);

    fp->flush();

    // Move pointer to the end of the file
    if (fileSize > 0) {
        fp->seek(fileSize);
    }
}

void FitHelper::WriteData(const void* data, FIT_UINT16 data_size, SDK::Interface::IFile* fp)
{
    FIT_UINT16 offset;

    size_t bw;
    fp->write(reinterpret_cast<const char*>(data), static_cast<size_t>(data_size), bw);
    fp->flush();

    for (offset = 0; offset < data_size; offset++) {
        mDataCRC = FitCRC_Get16(mDataCRC, *((FIT_UINT8*)data + offset));
    }
}

void FitHelper::WriteCRC(SDK::Interface::IFile* fp)
{
    size_t bw;
    fp->write(reinterpret_cast<const char*>(&mDataCRC), sizeof(FIT_UINT16), bw);
    fp->flush();
}

void FitHelper::WriteMessageDefinition(FIT_UINT8 local_mesg_number, const void* mesg_def_pointer, FIT_UINT16 mesg_def_size, SDK::Interface::IFile* fp)
{
    FIT_UINT8 header = local_mesg_number | FIT_HDR_TYPE_DEF_BIT;
    WriteData(&header, FIT_HDR_SIZE, fp);
    WriteData(mesg_def_pointer, mesg_def_size, fp);
}

void FitHelper::WriteMessageDefinitionWithDevFields(FIT_UINT8 local_mesg_number, const void* mesg_def_pointer, FIT_UINT16 mesg_def_size,
                                                         FIT_UINT8 number_dev_fields, FIT_DEV_FIELD_DEF* dev_field_definitions, SDK::Interface::IFile* fp)
{
    FIT_UINT16 i;
    FIT_UINT8 header = local_mesg_number | FIT_HDR_TYPE_DEF_BIT | FIT_HDR_DEV_DATA_BIT;
    WriteData(&header, FIT_HDR_SIZE, fp);
    WriteData(mesg_def_pointer, mesg_def_size, fp);

    WriteData(&number_dev_fields, sizeof(FIT_UINT8), fp);
    for (i = 0; i < number_dev_fields; i++) {
        WriteData(&dev_field_definitions[i], sizeof(FIT_DEV_FIELD_DEF), fp);
    }
}

void FitHelper::WriteMessage(FIT_UINT8 local_mesg_number, const void* mesg_pointer, FIT_UINT16 mesg_size, SDK::Interface::IFile* fp)
{
    WriteData(&local_mesg_number, FIT_HDR_SIZE, fp);
    WriteData(mesg_pointer, mesg_size, fp);
}

void FitHelper::WriteDeveloperField(const void* data, FIT_UINT16 data_size, SDK::Interface::IFile* fp)
{
    WriteData(data, data_size, fp);
}

}