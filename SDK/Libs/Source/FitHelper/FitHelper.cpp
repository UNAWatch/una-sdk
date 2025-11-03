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

    /**
     * @brief Construct a new FitHelper for a standard FIT message.
     *
     * @param msgID   Local message number (0–15) used in FIT header.
     * @param msgDef  Pointer to the FIT message definition from fit_mesg_defs[].
     */
    FitHelper::FitHelper(uint8_t msgID, FIT_MESG_DEF* msgDef)
	    : mInited(false)
        , mMsgID(msgID)
        , mMsgDefOrigin(msgDef)
	    , mMsgDefBuffer()
        , mMsgDef()
	    , mIsField(false)
        , mBaseType(FIT_FIT_BASE_TYPE_INVALID)
	    , mDevelopFieldSize(1)
    {
    }

    /**
     * @brief Construct a new FitHelper for a developer field and attach it to a parent container.
     *
     * @details
     * Initializes a FIELD_DESCRIPTION helper and automatically registers it
     * in the specified parent container. Each developer field is associated
     * with a fixed number of base-type elements (e.g., bytes for STRING fields).
     *
     * @param msgID        Local message number for this field.
     * @param container    Parent FitHelper to which this field belongs.
     * @param itemsCount   Number of elements (bytes) in the developer field.
     */
    FitHelper::FitHelper(uint8_t msgID, FitHelper& container, FIT_UINT8 itemsCount)
        : mInited(false)
        , mMsgID(msgID)
        , mMsgDefOrigin((FIT_MESG_DEF*)fit_mesg_defs[FIT_MESG_FIELD_DESCRIPTION])
        , mMsgDefBuffer()
        , mMsgDef()
        , mIsField(true)
        , mBaseType(FIT_FIT_BASE_TYPE_INVALID)
        , mDevelopFieldSize(itemsCount)
    {
        container.addField(this);
    }

    /**
     * @brief Initialize helper with a subset of fields from the base definition.
     *
     * @param fields  List of field numbers to include in the optimized message definition.
     * @return true   Initialization successful.
     * @return false  If already initialized, or if fields are invalid or duplicated.
     */
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

    /**
     * @brief Write the message definition (standard or with developer fields) to file.
     *
     * @param fp  Target file pointer.
     * @return true  If definition successfully written.
     */
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
                dev_field_def.size      = mFields[i]->getFieldSize() * mFields[i]->getBaseTypeSize();
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

    /**
     * @brief Write a data message instance to the FIT file.
     *
     * @param data  Pointer to the message data structure.
     * @param fp    Target file pointer.
     * @return true If message successfully written.
     */
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

        const uint8_t* buff = (const uint8_t*)data;
        for (uint8_t idx = 0; idx < mMsgFields.size(); ++idx) {
            WriteData(&buff[mMsgFields[idx].offset], mMsgFields[idx].size, fp);
        }

	    return true;
    }

    /**
     * @brief Print a FIT message definition for debugging.
     *
     * @param msgDef  Pointer to the message definition to print.
     */
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

    /**
     * @brief Attach a developer field to the current container.
     *
     * @param field  Pointer to the developer field helper.
     */
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

    /**
     * @brief Write data for a single developer field to the FIT file.
     *
     * @param idx   Index of the developer field.
     * @param data  Pointer to the field data.
     * @param fp    Target file pointer.
     */
    void FitHelper::writeFieldMessage(uint8_t idx, const void* data, SDK::Interface::IFile* fp)
    {
        if (idx >= mFields.size()) {
            return;
        }
 
        FitHelper* field = mFields[idx];

	    uint8_t fieldSize = field->getFieldSize() * field->getBaseTypeSize();

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

    /**
     * @brief Get declared developer field size.
     */
    FIT_UINT8 FitHelper::getFieldSize()
    {
        return mDevelopFieldSize;
    }

    /**
     * @brief Get base type size for the current message or field.
     */
    FIT_UINT8 FitHelper::getBaseTypeSize()
    {
        return getBaseTypeSize(mBaseType);
    }

    /**
     * @brief Get current base type.
     */
    FIT_FIT_BASE_TYPE FitHelper::getBaseType()
    {
	    return mBaseType;
    }

    /**
     * @brief Check if all provided field numbers are unique.
     */
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

    /**
     * @brief Validate that provided fields exist in the original message definition.
     */
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

    /**
     * @brief Build a reduced message definition based on selected fields.
     */
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

    /**
     * @brief Get size in bytes of a given base type.
     *
     * @param base_type  Base type identifier.
     * @return FIT_UINT8 Size in bytes of the base type.
	 */
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

    /**
     * @brief Build message field offset and size information.
	 */
    void FitHelper::makeMsgFields(std::initializer_list<FIT_EVENT_FIELD_NUM> fields)
    {
        if (fields.size() == 0) {
		    mMsgFields.clear();

            uint8_t size = 0;
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

    /**
     * @brief Get byte offset of a specific field within the message structure.
     *
     * @param field      Field number to locate.
     * @return uint8_t  Byte offset of the field, or UINT8_MAX if not found.
	 */
    uint8_t FitHelper::getFieldOffset(FIT_EVENT_FIELD_NUM field)
    {
        uint8_t offset = 0;
        for (FIT_UINT8 idx = 0; idx < mMsgDefOrigin->num_fields; ++idx) {
            if (field == mMsgDefOrigin->fields[FIT_MESG_DEF_FIELD_OFFSET(field_def_num, idx)]) {
                return offset;
            }
        
            FIT_UINT8 base_type_num = mMsgDefOrigin->fields[FIT_MESG_DEF_FIELD_OFFSET(base_type, idx)] & FIT_BASE_TYPE_NUM_MASK;
            if (base_type_num >= FIT_BASE_TYPES) {
                return UINT8_MAX;
            }

            FIT_UINT8 fieldSize = mMsgDefOrigin->fields[FIT_MESG_DEF_FIELD_OFFSET(size, idx)];
            offset += fieldSize;
        }
	
        return UINT8_MAX;
    }

    /**
     * @brief Get size in bytes of a specific field within the message structure.
     *
     * @param field    Field number to locate.
	 * @return uint8_t Size in bytes of the field, or UINT8_MAX if not found.
     */
    uint8_t FitHelper::getFieldSize(FIT_EVENT_FIELD_NUM field)
    {
        for (FIT_UINT8 idx = 0; idx < mMsgDefOrigin->num_fields; ++idx) {
            if (field == mMsgDefOrigin->fields[FIT_MESG_DEF_FIELD_OFFSET(field_def_num, idx)]) {
                FIT_UINT8 base_type_num = mMsgDefOrigin->fields[FIT_MESG_DEF_FIELD_OFFSET(base_type, idx)] & FIT_BASE_TYPE_NUM_MASK;
                if (base_type_num >= FIT_BASE_TYPES) {
                    return UINT8_MAX;
                }

                FIT_UINT8 fieldSize = mMsgDefOrigin->fields[FIT_MESG_DEF_FIELD_OFFSET(size, idx)];

                return fieldSize;
            }
        }

        return UINT8_MAX;
    }

    /**
     * @brief Write raw data to the FIT file.
     *
     * @param data        Pointer to the data to write.
     * @param data_size   Size of the data in bytes.
     * @param fp          Target file pointer.
	 */
    void FitHelper::WriteData(const void* data, FIT_UINT16 data_size, SDK::Interface::IFile* fp)
    {
        size_t bw;
        fp->write(reinterpret_cast<const char*>(data), static_cast<size_t>(data_size), bw);
        fp->flush();
    }

    /**
     * @brief Write a data message to the FIT file.
     *
     * @param local_mesg_number  Local message number (0–15).
     * @param mesg_pointer       Pointer to the message data structure.
	 * @param mesg_size          Size of the message data in bytes.
     */
    void FitHelper::WriteMessage(FIT_UINT8 local_mesg_number, const void* mesg_pointer, FIT_UINT16 mesg_size, SDK::Interface::IFile* fp)
    {
        WriteData(&local_mesg_number, FIT_HDR_SIZE, fp);
        WriteData(mesg_pointer, mesg_size, fp);
    }

}
