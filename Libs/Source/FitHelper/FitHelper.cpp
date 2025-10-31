/**
 ******************************************************************************
 * @file    FitHelper.hpp
 * @date    28-October-2025
 * @brief   FIT message helper class
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "SDK/FitHelper/FitHelper.hpp"

#include <unordered_set>

#define LOG_MODULE_PRX      "FitHelper"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace SDK::Component {

FitHelper::FitHelper(uint8_t msgID, FIT_MESG_DEF* msgDef)
	: mInited(false)
    , mMsgID(msgID)
    , mMsgDefOrigin(msgDef)
	, mMsgDefBuffer()
    , mMsgDef()
	, mDataCRC(0)
	, mIsField(msgDef == fit_mesg_defs[FIT_MESG_FIELD_DESCRIPTION])
    , mBaseType(FIT_FIT_BASE_TYPE_INVALID)
{
}

bool FitHelper::init(std::initializer_list<FIT_UINT8> fields)
{
    if (mInited) {
        return false;
    }

    if (fields.size()) {
        if (!isUnique(fields)) {
            return false;
        }

        if (!isValid(fields)) {
            return false;
        }
    }

	makeMsgDef(fields);

    makeMsgFields(fields);

    mInited = true;

    return true;
}

bool FitHelper::writeDef(SDK::Interface::IFile* fp)
{
    if (!mInited) {
        return false;
	}

    if (mFields.size()) {
        FIT_UINT8 header = mMsgID | FIT_HDR_TYPE_DEF_BIT | FIT_HDR_DEV_DATA_BIT;
        WriteData(&header, FIT_HDR_SIZE, fp);
        WriteData((const void*)mMsgDef,
                  static_cast<FIT_UINT16>(sizeof(FIT_MESG_DEF) - FIT_FLEX_ARRAY + mMsgDef->num_fields * FIT_FIELD_DEF_SIZE),
                  fp);

        FIT_UINT8 numberFields = static_cast<FIT_UINT8>(mFields.size());
        WriteData(&numberFields, sizeof(FIT_UINT8), fp);

        for (FIT_UINT8 i = 0; i < numberFields; i++) {
            FIT_DEV_FIELD_DEF dev_field_def{};

            dev_field_def.def_num   = i;
            dev_field_def.size      = mFields[i]->getBaseTypeSize();
            dev_field_def.dev_index = 0;

            WriteData(&dev_field_def, sizeof(FIT_DEV_FIELD_DEF), fp);
        }

    } else {
        FIT_UINT8 header = mMsgID | FIT_HDR_TYPE_DEF_BIT;
        WriteData(&header, FIT_HDR_SIZE, fp);
        WriteData((const void*)mMsgDef,
                  static_cast<FIT_UINT16>(sizeof(FIT_MESG_DEF) - FIT_FLEX_ARRAY + mMsgDef->num_fields * FIT_FIELD_DEF_SIZE),
                  fp);
    }

    return true;
}

bool FitHelper::writeMessage(void* data, SDK::Interface::IFile * fp)
{
    if (!mInited) {
		return false;
	}

    if (mIsField) {
        FIT_FIELD_DESCRIPTION_MESG* msg = (FIT_FIELD_DESCRIPTION_MESG*)data;
        mBaseType = (FIT_FIT_BASE_TYPE)msg->fit_base_type_id;
    }

    WriteData(&mMsgID, FIT_HDR_SIZE, fp);

    //const uint8_t* buff = (const uint8_t*)data;
    //for (uint8_t idx = 0; idx < mMsgDef->num_fields; ++idx) {
    //    WriteData(&buff[mMsgFields[idx].offset], mMsgFields[idx].size, fp);
    //}

    const uint8_t* buff = (const uint8_t*)data;
    for (uint8_t idx = 0; idx < mMsgFields.size(); ++idx) {
        WriteData(&buff[mMsgFields[idx].offset], mMsgFields[idx].size, fp);
    }

	return true;
}

void FitHelper::printMsgDef(const FIT_MESG_DEF* msgDef)
{
    LOG_INFO("reserved_1      = %d\n", (int) msgDef->reserved_1);
    LOG_INFO("arch            = %d\n", (int) msgDef->arch);
    LOG_INFO("global_mesg_num = %d\n", (int) msgDef->global_mesg_num);
    LOG_INFO("num_fields      = %d\n", (int) msgDef->num_fields);

    LOG_INFO("  ID SIZE TYPE\n");
    for (uint8_t idx = 0; idx < msgDef->num_fields; ++idx) {
        LOG_INFO("%4d %4d %4d\n", (int)msgDef->fields[FIT_MESG_DEF_FIELD_OFFSET(field_def_num, idx)], 
                                  (int)msgDef->fields[FIT_MESG_DEF_FIELD_OFFSET(size, idx)],
                                  (int)msgDef->fields[FIT_MESG_DEF_FIELD_OFFSET(base_type, idx)]);
    }
}

void FitHelper::addField(FitHelper* field)
{
    if (field == nullptr) {
        return;
	}

    if (std::find(mFields.begin(), mFields.end(), field) != mFields.end()) {
        return;
    }

    mFields.push_back(field);
}

void FitHelper::writeFieldMessage(uint8_t idx, const void* data, SDK::Interface::IFile* fp)
{
    if (idx >= mFields.size()) {
        return;
    }
 
    FitHelper* field = mFields[idx];

	uint8_t fieldSize;
    if (field->getBaseType() == FIT_FIT_BASE_TYPE_STRING) {
		const char* str = (const char*)data;
		fieldSize = static_cast<uint8_t>(strlen(str));
	} else {
        fieldSize = field->getBaseTypeSize();
    }

    WriteData(data, fieldSize, fp);
}

FIT_UINT8 FitHelper::getBaseTypeSize()
{
	return getBaseTypeSize(mBaseType);
}

FIT_FIT_BASE_TYPE FitHelper::getBaseType()
{
	return mBaseType;
}

bool FitHelper::isUnique(std::initializer_list<FIT_EVENT_FIELD_NUM> fields)
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

bool FitHelper::isValid(std::initializer_list<FIT_EVENT_FIELD_NUM> fields)
{
    for (auto f : fields) {
		bool ok = false;
        for (uint8_t idx = 0; idx < mMsgDefOrigin->num_fields; ++idx) {
            if (f == mMsgDefOrigin->fields[idx * FIT_FIELD_DEF_SIZE]) {
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

void FitHelper::makeMsgDef(std::initializer_list<FIT_EVENT_FIELD_NUM> fields)
{
    if (fields.size() == 0) {
        mMsgDef = mMsgDefOrigin;
        printMsgDef((const FIT_MESG_DEF*)mMsgDef);
        return;
    }

    mMsgDefBuffer = std::make_unique<uint8_t[]>(sizeof(FIT_MESG_DEF) - FIT_FLEX_ARRAY + fields.size() * FIT_FIELD_DEF_SIZE);

    mMsgDef = (FIT_MESG_DEF*) mMsgDefBuffer.get();

    mMsgDef->reserved_1      = mMsgDefOrigin->reserved_1;
    mMsgDef->arch            = mMsgDefOrigin->arch;
    mMsgDef->global_mesg_num = mMsgDefOrigin->global_mesg_num;
    mMsgDef->num_fields      = static_cast<FIT_UINT8>(fields.size());

	uint8_t offset = 0;

    for (auto f : fields) {
        for (uint8_t idx = 0; idx < mMsgDefOrigin->num_fields; ++idx) {
            if (f == mMsgDefOrigin->fields[FIT_MESG_DEF_FIELD_OFFSET(field_def_num, idx)]) {
                uint8_t field_def_num_dst = FIT_MESG_DEF_FIELD_OFFSET(field_def_num, offset);
                uint8_t size_dst          = FIT_MESG_DEF_FIELD_OFFSET(size, offset);
                uint8_t base_type_dst     = FIT_MESG_DEF_FIELD_OFFSET(base_type, offset);

                uint8_t field_def_num_src = FIT_MESG_DEF_FIELD_OFFSET(field_def_num, idx);
                uint8_t size_src          = FIT_MESG_DEF_FIELD_OFFSET(size, idx);
                uint8_t base_type_src     = FIT_MESG_DEF_FIELD_OFFSET(base_type, idx);

                mMsgDef->fields[field_def_num_dst] = mMsgDefOrigin->fields[field_def_num_src];
                mMsgDef->fields[size_dst]          = mMsgDefOrigin->fields[size_src];
                mMsgDef->fields[base_type_dst]     = mMsgDefOrigin->fields[base_type_src];

                ++offset;

                break;
            }
        }
    }

    LOG_INFO("Optimized MsgDef\n");
    printMsgDef((const FIT_MESG_DEF*)mMsgDef);
}

FIT_UINT8 FitHelper::getBaseTypeSize(FIT_FIT_BASE_TYPE base_type)
{
    switch (base_type) {
    case FIT_FIT_BASE_TYPE_ENUM:
    case FIT_FIT_BASE_TYPE_BYTE:
    case FIT_FIT_BASE_TYPE_SINT8:
    case FIT_FIT_BASE_TYPE_UINT8Z:
    case FIT_FIT_BASE_TYPE_UINT8:   return 1;

    case FIT_FIT_BASE_TYPE_SINT16:
    case FIT_FIT_BASE_TYPE_UINT16Z:
    case FIT_FIT_BASE_TYPE_UINT16:  return 2;

    case FIT_FIT_BASE_TYPE_FLOAT32:
    case FIT_FIT_BASE_TYPE_SINT32:
    case FIT_FIT_BASE_TYPE_UINT32Z:
    case FIT_FIT_BASE_TYPE_UINT32:  return 4;

    case FIT_FIT_BASE_TYPE_FLOAT64:
    case FIT_FIT_BASE_TYPE_SINT64:
    case FIT_FIT_BASE_TYPE_UINT64:
    case FIT_FIT_BASE_TYPE_UINT64Z: return 8;

    case FIT_FIT_BASE_TYPE_STRING:  return 1;

    default:                        return 0;
    }
}

void FitHelper::makeMsgFields(std::initializer_list<FIT_EVENT_FIELD_NUM> fields)
{
    //if (mIsField) {
    //    mMsgFields = std::make_unique<MsgField[]>(1);

    //    uint16_t size = 0;
    //    for (uint8_t idx = 0; idx < mMsgDefOrigin->num_fields; ++idx) {
    //        size += mMsgDefOrigin->fields[FIT_MESG_DEF_FIELD_OFFSET(size, idx)];
    //    }

    //    mMsgFields[0].offset = 0;
    //    mMsgFields[0].size   = size;

    //    return;
    //}

    if (fields.size() == 0) {
		mMsgFields.clear();

        uint16_t size = 0;
        for (uint8_t idx = 0; idx < mMsgDefOrigin->num_fields; ++idx) {
            size += mMsgDefOrigin->fields[FIT_MESG_DEF_FIELD_OFFSET(size, idx)];
        }

		mMsgFields.push_back(MsgField{ 0, size });
    } else {
        mMsgFields.clear();
        uint16_t idx = 0;
        for (auto f : fields) {
			MsgField item{};

            item.offset = getFieldOffset(f);
            item.size   = getFieldSize(f);
            
            mMsgFields.push_back(item);
            
            ++idx;
        }
    }
}

uint16_t FitHelper::getFieldOffset(FIT_EVENT_FIELD_NUM field)
{
    uint16_t offset = 0;
    for (FIT_UINT8 idx = 0; idx < mMsgDefOrigin->num_fields; ++idx) {
        if (field == mMsgDefOrigin->fields[FIT_MESG_DEF_FIELD_OFFSET(field_def_num, idx)]) {
            return offset;
        }
        
        FIT_UINT8 base_type_num = mMsgDefOrigin->fields[FIT_MESG_DEF_FIELD_OFFSET(base_type, idx)] & FIT_BASE_TYPE_NUM_MASK;
        if (base_type_num >= FIT_BASE_TYPES) {
            return UINT16_MAX;
        }

        FIT_UINT8 fieldSize = mMsgDefOrigin->fields[FIT_MESG_DEF_FIELD_OFFSET(size, idx)];
        offset += fieldSize;
 
        //FIT_UINT8 base_type_size = fit_base_type_sizes[base_type_num];
        //FIT_UINT8 fieldSize      = mMsgDefOrigin->fields[FIT_MESG_DEF_FIELD_OFFSET(size, idx)];
        //offset += fieldSize * base_type_size;
    }
	
    return UINT16_MAX;
}

uint16_t FitHelper::getFieldSize(FIT_EVENT_FIELD_NUM field)
{
    for (FIT_UINT8 idx = 0; idx < mMsgDefOrigin->num_fields; ++idx) {
        if (field == mMsgDefOrigin->fields[FIT_MESG_DEF_FIELD_OFFSET(field_def_num, idx)]) {
            FIT_UINT8 base_type_num = mMsgDefOrigin->fields[FIT_MESG_DEF_FIELD_OFFSET(base_type, idx)] & FIT_BASE_TYPE_NUM_MASK;
            if (base_type_num >= FIT_BASE_TYPES) {
                return UINT16_MAX;
            }

            FIT_UINT8 fieldSize = mMsgDefOrigin->fields[FIT_MESG_DEF_FIELD_OFFSET(size, idx)];

            return fieldSize;

            //FIT_UINT8 base_type_size = fit_base_type_sizes[base_type_num];
            //FIT_UINT8 fieldSize      = mMsgDefOrigin->fields[FIT_MESG_DEF_FIELD_OFFSET(size, idx)];

            //return fieldSize * base_type_size;
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

    //if (fileSize > FIT_FILE_HDR_SIZE - sizeof(FIT_UINT16)) {
    //    file_header.data_size = static_cast<FIT_UINT32>(fileSize - FIT_FILE_HDR_SIZE - sizeof(FIT_UINT16));
    //}
    //else {
    //    file_header.data_size = 0;
    //}

    if (fileSize > FIT_FILE_HDR_SIZE) {
        file_header.data_size = static_cast<FIT_UINT32>(fileSize - FIT_FILE_HDR_SIZE);
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
    //FIT_UINT16 offset;

    size_t bw;
    fp->write(reinterpret_cast<const char*>(data), static_cast<size_t>(data_size), bw);
    fp->flush();

    mDataCRC = FitCRC_Update16(mDataCRC, data, data_size);

    //for (offset = 0; offset < data_size; offset++) {
    //    mDataCRC = FitCRC_Get16(mDataCRC, *((FIT_UINT8*)data + offset));
    //}
}

void FitHelper::WriteCRC(SDK::Interface::IFile* fp)
{
	fp->close();

    fp->open(false);

    FIT_UINT8 buffer[512];
    size_t    size = fp->size();
    size_t    pos  = 0;
	uint16_t  crc  = 0;

    while (pos < size) {
        size_t toRead = size - pos;
        if (toRead > sizeof(buffer)) {
            toRead = sizeof(buffer);
        }

        size_t br;
        fp->read(reinterpret_cast<char*>(buffer), toRead, br);

        crc = FitCRC_Update16(crc, buffer, static_cast<FIT_UINT32>(br));

		pos += br;
	}

    fp->close();

    fp->open(true, false);

    size_t bw;
    fp->write(reinterpret_cast<const char*>(&crc), sizeof(FIT_UINT16), bw);
    fp->flush();


    //size_t bw;
    //fp->write(reinterpret_cast<const char*>(&mDataCRC), sizeof(FIT_UINT16), bw);
    //fp->flush();
}

void FitHelper::WriteMessageDefinition(FIT_UINT8 local_mesg_number, const void* mesg_def_pointer, FIT_UINT16 mesg_def_size, SDK::Interface::IFile* fp)
{
    FIT_UINT8 header = local_mesg_number | FIT_HDR_TYPE_DEF_BIT;
    WriteData(&header, FIT_HDR_SIZE, fp);
    WriteData(mesg_def_pointer, mesg_def_size, fp);
}

void FitHelper::WriteMessageDefinitionWithDevFields(FIT_UINT8              local_mesg_number,
                                                    const void*            mesg_def_pointer,
                                                    FIT_UINT16             mesg_def_size,
                                                    FIT_UINT8              number_dev_fields,
                                                    FIT_DEV_FIELD_DEF*     dev_field_definitions,
                                                    SDK::Interface::IFile* fp)
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