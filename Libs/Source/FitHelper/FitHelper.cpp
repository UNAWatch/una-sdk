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
#include <algorithm>

#define LOG_MODULE_PRX      "FitHelper"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace SDK::Component {

    FitHelper::FitHelper(uint8_t msgID, const FIT_MESG_DEF* msgDef)
	    : mInited(false)
        , mMsgID(msgID)
        , mMsgDefOrigin(msgDef)
	    , mMsgDefBuffer()
        , mMsgDef()
	    , mIsField(false)
        , mBaseType(FIT_FIT_BASE_TYPE_INVALID)
	    , mDevelopItemsCount(1)
		, mDevelopIndex(0)
		, mUserFields()
		, mFieldID(0)
    {
    }

    FitHelper::FitHelper(uint8_t                           msgID,
                         uint8_t                           fieldID,
                         std::initializer_list<FitHelper*> container,
                         FIT_UINT8                         itemsCount,
                         FIT_UINT8                         devIndex)
        : mInited(false)
        , mMsgID(msgID)
        , mMsgDefOrigin(fit_mesg_defs[FIT_MESG_FIELD_DESCRIPTION])
        , mMsgDefBuffer()
        , mMsgDef()
        , mIsField(true)
        , mBaseType(FIT_FIT_BASE_TYPE_INVALID)
        , mDevelopItemsCount(itemsCount == 0 ? 1 : itemsCount)
        , mDevelopIndex(devIndex)
        , mUserFields()
        , mFieldID(fieldID)
    {
		for (auto c : container) {
            if (c) {
                c->addField(this);
            }
        }
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

        if (mUserFields.size()) {
            FIT_UINT8 header = mMsgID | FIT_HDR_TYPE_DEF_BIT | FIT_HDR_DEV_DATA_BIT;
            WriteData(&header, FIT_HDR_SIZE, fp);
            WriteData((const void*)mMsgDef,
                      static_cast<FIT_UINT16>(sizeof(FIT_MESG_DEF) - FIT_FLEX_ARRAY + mMsgDef->num_fields * FIT_FIELD_DEF_SIZE),
                      fp);

            FIT_UINT8 numberFields = static_cast<FIT_UINT8>(mUserFields.size());
            WriteData(&numberFields, sizeof(FIT_UINT8), fp);

            for (FIT_UINT8 i = 0; i < numberFields; i++) {
                FIT_DEV_FIELD_DEF dev_field_def{};

                dev_field_def.def_num   = mUserFields[i]->getFieldID();
                dev_field_def.size      = mUserFields[i]->getFieldSize();
                dev_field_def.dev_index = mDevelopIndex;

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

    bool FitHelper::writeMessage(const void* data, SDK::Interface::IFile * fp)
    {
        if (!mInited) {
		    return false;
	    }

        if (mIsField) {
            const FIT_FIELD_DESCRIPTION_MESG* msg = static_cast<const FIT_FIELD_DESCRIPTION_MESG*>(data);
            mBaseType = (FIT_FIT_BASE_TYPE)msg->fit_base_type_id;
        }

        WriteData(&mMsgID, FIT_HDR_SIZE, fp);

        const uint8_t* buff = (const uint8_t*)data;
        for (uint8_t idx = 0; idx < mMsgFields.size(); ++idx) {
            WriteData(&buff[mMsgFields[idx].offset], mMsgFields[idx].size, fp);
        }

	    return true;
    }

    bool FitHelper::writeFieldDescription(const char* name, const char* units,
                                          FIT_UINT8 baseType, SDK::Interface::IFile* fp)
    {
        if (name == nullptr) {
            name = "";
        }

        // String field byte counts, including the trailing NUL. Capped at the
        // FIT example-profile buffer sizes (field_name 64, units 16).
        auto cappedLen = [](const char* s, FIT_UINT8 cap) -> FIT_UINT8 {
            FIT_UINT8 n = 0;
            while (n < cap && s[n] != '\0') {
                ++n;
            }
            return n;
        };
        const FIT_UINT8 nameChars  = cappedLen(name, 63);
        const bool      hasUnits   = (units != nullptr && units[0] != '\0');
        const FIT_UINT8 unitsChars = hasUnits ? cappedLen(units, 15) : 0;
        const FIT_UINT8 nameSize   = static_cast<FIT_UINT8>(nameChars + 1);
        const FIT_UINT8 unitsSize  = static_cast<FIT_UINT8>(unitsChars + 1);
        const FIT_UINT8 numFields  = hasUnits ? 5 : 4;

        // Record the developer field's base type so the parent record's
        // developer-field definition (getFieldSize) is sized correctly — this
        // is what writeMessage() would normally set.
        mBaseType = static_cast<FIT_FIT_BASE_TYPE>(baseType);

        // --- definition record ---
        const FIT_UINT8 defHeader = static_cast<FIT_UINT8>(mMsgID | FIT_HDR_TYPE_DEF_BIT);
        WriteData(&defHeader, FIT_HDR_SIZE, fp);

        const FIT_UINT8 defFixed[] = {
            0,                                                                    // reserved
            FIT_ARCH_ENDIAN_LITTLE,                                               // arch
            static_cast<FIT_UINT8>(FIT_MESG_NUM_FIELD_DESCRIPTION & 0xFF),        // global mesg num (LE)
            static_cast<FIT_UINT8>((FIT_MESG_NUM_FIELD_DESCRIPTION >> 8) & 0xFF),
            numFields,
            // field defs: { field_def_num, size, base_type }
            FIT_FIELD_DESCRIPTION_FIELD_NUM_DEVELOPER_DATA_INDEX,    1,        FIT_BASE_TYPE_UINT8,
            FIT_FIELD_DESCRIPTION_FIELD_NUM_FIELD_DEFINITION_NUMBER, 1,        FIT_BASE_TYPE_UINT8,
            FIT_FIELD_DESCRIPTION_FIELD_NUM_FIT_BASE_TYPE_ID,        1,        FIT_BASE_TYPE_UINT8,
            FIT_FIELD_DESCRIPTION_FIELD_NUM_FIELD_NAME,             nameSize,  FIT_BASE_TYPE_STRING,
        };
        WriteData(defFixed, sizeof(defFixed), fp);
        if (hasUnits) {
            const FIT_UINT8 unitsDef[] = {
                FIT_FIELD_DESCRIPTION_FIELD_NUM_UNITS, unitsSize, FIT_BASE_TYPE_STRING,
            };
            WriteData(unitsDef, sizeof(unitsDef), fp);
        }

        // --- data record ---
        const FIT_UINT8 dataHeader = mMsgID;
        WriteData(&dataHeader, FIT_HDR_SIZE, fp);

        const FIT_UINT8 vals[] = { mDevelopIndex, mFieldID, baseType };
        WriteData(vals, sizeof(vals), fp);

        char nameBuf[64] = {};
        for (FIT_UINT8 i = 0; i < nameChars; ++i) {
            nameBuf[i] = name[i];
        }
        WriteData(nameBuf, nameSize, fp);

        if (hasUnits) {
            char unitsBuf[16] = {};
            for (FIT_UINT8 i = 0; i < unitsChars; ++i) {
                unitsBuf[i] = units[i];
            }
            WriteData(unitsBuf, unitsSize, fp);
        }

        return true;
    }

    void FitHelper::printMsgDef(const FIT_MESG_DEF* msgDef)
    {
#if LOG_MODULE_LEVEL == LOG_LEVEL_DEBUG
        LOG_DEBUG("reserved_1      = %d\n", (int) msgDef->reserved_1);
        LOG_DEBUG("arch            = %d\n", (int) msgDef->arch);
        LOG_DEBUG("global_mesg_num = %d\n", (int) msgDef->global_mesg_num);
        LOG_DEBUG("num_fields      = %d\n", (int) msgDef->num_fields);

        LOG_DEBUG("  ID SIZE TYPE\n");
        for (uint8_t idx = 0; idx < msgDef->num_fields; ++idx) {
            LOG_DEBUG("%4d %4d %4d\n", (int)msgDef->fields[FIT_MESG_DEF_FIELD_OFFSET(field_def_num, idx)],
                                       (int)msgDef->fields[FIT_MESG_DEF_FIELD_OFFSET(size, idx)],
                                       (int)msgDef->fields[FIT_MESG_DEF_FIELD_OFFSET(base_type, idx)]);
        }
#else
        (void) msgDef;
#endif
    }

    void FitHelper::addField(FitHelper* field)
    {
        if (field == nullptr) {
            return;
	    }

        if (std::find(mUserFields.begin(), mUserFields.end(), field) != mUserFields.end()) {
            return;
        }

        mUserFields.push_back(field);
    }

    void FitHelper::writeFieldMessage(uint8_t idx, const void* data, SDK::Interface::IFile* fp)
    {
        if (idx >= mUserFields.size()) {
            return;
        }
 
        FitHelper* field = mUserFields[idx];

	    uint8_t fieldSize = field->getFieldSize();

        if (field->getBaseType() == FIT_FIT_BASE_TYPE_STRING) {
		    const char* str = (const char*)data;
            uint8_t len = static_cast<uint8_t>(strlen(str));

            if (len > fieldSize) {
                len = fieldSize;
		    }

		    char padding[255]{};
		    strcpy(padding, str);
		    WriteData(padding, fieldSize, fp);
            return;
	    }

        WriteData(data, fieldSize, fp);
    }

    FIT_UINT8 FitHelper::getItemsCount() const
    {
        return mDevelopItemsCount;
    }

    uint8_t FitHelper::getFieldID() const
    {
        return mFieldID;
    }

    FIT_UINT8 FitHelper::getFieldSize() const
    {
        return getItemsCount() * getBaseTypeSize();
    }

    FIT_UINT8 FitHelper::getBaseTypeSize() const
    {
        return getBaseTypeSize(mBaseType);
    }

    FIT_FIT_BASE_TYPE FitHelper::getBaseType() const
    {
	    return mBaseType;
    }

    bool FitHelper::isUnique(std::initializer_list<FIT_EVENT_FIELD_NUM> fields) const
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

    bool FitHelper::isValid(std::initializer_list<FIT_EVENT_FIELD_NUM> fields) const
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

        FIT_MESG_DEF* writable = reinterpret_cast<FIT_MESG_DEF*>(mMsgDefBuffer.get());
        mMsgDef = writable;

        writable->reserved_1      = mMsgDefOrigin->reserved_1;
        writable->arch            = mMsgDefOrigin->arch;
        writable->global_mesg_num = mMsgDefOrigin->global_mesg_num;
        writable->num_fields      = static_cast<FIT_UINT8>(fields.size());

	    uint8_t offset = 0;

        for (auto f : fields) {
            for (uint8_t idx = 0; idx < mMsgDefOrigin->num_fields; ++idx) {
                if (f == mMsgDefOrigin->fields[FIT_MESG_DEF_FIELD_OFFSET(field_def_num, idx)]) {
                    // 16-bit byte offsets into the def buffer.  Profiles like
                    // RELEASE have lap/session defs with >85 fields, so 3*idx
                    // overflows uint8_t for high-index fields (e.g. avg_heart_rate
                    // sits at idx 88 in the release lap def -> 264 bytes).
                    uint16_t field_def_num_dst = FIT_MESG_DEF_FIELD_OFFSET(field_def_num, offset);
                    uint16_t size_dst          = FIT_MESG_DEF_FIELD_OFFSET(size, offset);
                    uint16_t base_type_dst     = FIT_MESG_DEF_FIELD_OFFSET(base_type, offset);

                    uint16_t field_def_num_src = FIT_MESG_DEF_FIELD_OFFSET(field_def_num, idx);
                    uint16_t size_src          = FIT_MESG_DEF_FIELD_OFFSET(size, idx);
                    uint16_t base_type_src     = FIT_MESG_DEF_FIELD_OFFSET(base_type, idx);

                    writable->fields[field_def_num_dst] = mMsgDefOrigin->fields[field_def_num_src];
                    writable->fields[size_dst]          = mMsgDefOrigin->fields[size_src];
                    writable->fields[base_type_dst]     = mMsgDefOrigin->fields[base_type_src];

                    ++offset;

                    break;
                }
            }
        }

        LOG_DEBUG("Optimized MsgDef\n");
        printMsgDef((const FIT_MESG_DEF*)mMsgDef);
    }

    FIT_UINT8 FitHelper::getBaseTypeSize(FIT_FIT_BASE_TYPE base_type) const
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
        if (fields.size() == 0) {
		    mMsgFields.clear();

            // 16-bit accumulator: full message structs in newer profiles can
            // exceed 255 bytes (e.g. FIT_SESSION_MESG = 340 bytes).
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

    uint16_t FitHelper::getFieldOffset(FIT_EVENT_FIELD_NUM field) const
    {
        // 16-bit accumulator -- C message structs in newer profiles can be
        // >255 bytes (e.g. FIT_SESSION_MESG = 340 bytes), so summing per-field
        // sizes overflows uint8_t and produces wildly wrong byte offsets when
        // the writer later reads field data out of the caller's struct.
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
        }
	
        return UINT16_MAX;
    }

    uint8_t FitHelper::getFieldSize(FIT_EVENT_FIELD_NUM field) const
    {
        for (FIT_UINT8 idx = 0; idx < mMsgDefOrigin->num_fields; ++idx) {
            if (field == mMsgDefOrigin->fields[FIT_MESG_DEF_FIELD_OFFSET(field_def_num, idx)]) {
                FIT_UINT8 base_type_num = mMsgDefOrigin->fields[FIT_MESG_DEF_FIELD_OFFSET(base_type, idx)] & FIT_BASE_TYPE_NUM_MASK;
                if (base_type_num >= FIT_BASE_TYPES) {
                    return UINT8_MAX;
                }

                return mMsgDefOrigin->fields[FIT_MESG_DEF_FIELD_OFFSET(size, idx)];
            }
        }

        return UINT8_MAX;
    }

    void FitHelper::WriteData(const void* data, FIT_UINT16 data_size, SDK::Interface::IFile* fp)
    {
        size_t bw;
        fp->write(reinterpret_cast<const char*>(data), static_cast<size_t>(data_size), bw);
        fp->flush();
    }

    void FitHelper::WriteMessage(FIT_UINT8 local_mesg_number, const void* mesg_pointer, FIT_UINT16 mesg_size, SDK::Interface::IFile* fp)
    {
        WriteData(&local_mesg_number, FIT_HDR_SIZE, fp);
        WriteData(mesg_pointer, mesg_size, fp);
    }

}
