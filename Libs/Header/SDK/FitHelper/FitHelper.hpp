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

#ifndef __FIT_HELPER_HPP
#define __FIT_HELPER_HPP

#include "SDK/Interfaces/IFileSystem.hpp"

extern "C" {
#include "fit_product.h"
#include "fit_crc.h"
}

#include <cstdint>
#include <cstdbool>
#include <initializer_list>
#include <memory>
#include <vector>

namespace SDK::Component {

    class FitHelper
    {
    public:
        /**
         * @brief Construct a new FitHelper for a standard FIT message.
         *
         * @param msgID   Local message number (0–15) used in FIT header.
         * @param msgDef  Pointer to the FIT message definition from fit_mesg_defs[].
         */
        FitHelper(uint8_t msgID, FIT_MESG_DEF* msgDef);
        /**
         * @brief Construct a new FitHelper for a developer field and attach it to a parent container.
         *
         * @details
         * Initializes a FIELD_DESCRIPTION helper and automatically registers it
         * in the specified parent container. Each developer field is associated
         * with a fixed number of base-type elements (e.g., bytes for STRING fields).
         *
         * @param msgID      Local message number for this field.
         * @param container  Parent FitHelpers list to which this field belongs.
         * @param itemsCount Number of elements.
         * @param devIndex   Developer index.
         */
        FitHelper(uint8_t                           msgID,
                  std::initializer_list<FitHelper*> container,
                  FIT_UINT8                         itemsCount = 1,
                  FIT_UINT8                         devIndex = 0);

        /**
         * @brief Initialize helper with a subset of fields from the base definition.
         *
         * @param fields  List of field numbers to include in the optimized message definition.
         * @return true   Initialization successful.
         * @return false  If already initialized, or if fields are invalid or duplicated.
         */
        bool init(std::initializer_list<FIT_UINT8> fields = {});

        /**
         * @brief Write the message definition (standard or with developer fields) to file.
         *
         * @param fp  Target file pointer.
         * @return true  If definition successfully written.
         */
        bool writeDef(SDK::Interface::IFile* fp);

        /**
         * @brief Write a data message instance to the FIT file.
         *
         * @param data  Pointer to the message data structure.
         * @param fp    Target file pointer.
         * @return true If message successfully written.
         */
        bool writeMessage(void* data, SDK::Interface::IFile* fp);

        /**
         * @brief Print a FIT message definition for debugging.
         *
         * @param msgDef  Pointer to the message definition to print.
         */
        void printMsgDef(const FIT_MESG_DEF* msgDef);

		/**
		 * @brief Attach a developer field to the current container.
		 *
		 * @param field  Pointer to the developer field helper.
		 */
		void addField(FitHelper* field);

		/**
		 * @brief Write data for a single developer field to the FIT file.
		 *
		 * @param idx   Index of the developer field.
		 * @param data  Pointer to the field data.
		 * @param fp    Target file pointer.
		 */
		void writeFieldMessage(uint8_t idx, const void* data, SDK::Interface::IFile* fp);

        /**
         * @brief Get declared developer field size.
         */
        FIT_UINT8         getItemsCount();
        /**
         * @brief Get field size.
         */
        FIT_UINT8         getFieldSize();
        /**
         * @brief Get base type size for the current message or field.
         */
        FIT_UINT8         getBaseTypeSize();
        /**
         * @brief Get current base type.
         */
        FIT_FIT_BASE_TYPE getBaseType();

    private:
        FitHelper(const FitHelper&)            = delete;
        FitHelper& operator=(const FitHelper&) = delete;
        
        FitHelper(FitHelper&&)            = delete;
        FitHelper& operator=(FitHelper&&) = delete;

        /**
         * @brief Get size in bytes of a given base type.
         *
         * @param base_type  Base type identifier.
         * @return FIT_UINT8 Size in bytes of the base type.
         */
        FIT_UINT8 getBaseTypeSize(FIT_FIT_BASE_TYPE base_type);

        /**
         * @brief Check if all provided field numbers are unique.
         */
        bool isUnique(std::initializer_list<FIT_EVENT_FIELD_NUM> fields);
        /**
         * @brief Validate that provided fields exist in the original message definition.
         */
        bool isValid(std::initializer_list<FIT_EVENT_FIELD_NUM> fields);
        /**
         * @brief Build a reduced message definition based on selected fields.
         */
        void makeMsgDef(std::initializer_list<FIT_EVENT_FIELD_NUM> fields);
        /**
         * @brief Build message field offset and size information.
         */
        void makeMsgFields(std::initializer_list<FIT_EVENT_FIELD_NUM> fields);

        /**
         * @brief Get byte offset of a specific field within the message structure.
         *
         * @param field      Field number to locate.
         * @return uint8_t  Byte offset of the field, or UINT8_MAX if not found.
         */
        uint8_t getFieldOffset(FIT_EVENT_FIELD_NUM field);
        /**
         * @brief Get size in bytes of a specific field within the message structure.
         *
         * @param field    Field number to locate.
         * @return uint8_t Size in bytes of the field, or UINT8_MAX if not found.
         */
        uint8_t getFieldSize(FIT_EVENT_FIELD_NUM field);

        /**
         * @brief Write raw data to the FIT file.
         *
         * @param data        Pointer to the data to write.
         * @param data_size   Size of the data in bytes.
         * @param fp          Target file pointer.
         */
        void WriteData(const void* data, FIT_UINT16 data_size, SDK::Interface::IFile* fp);
        /**
         * @brief Write a data message to the FIT file.
         *
         * @param local_mesg_number  Local message number (0–15).
         * @param mesg_pointer       Pointer to the message data structure.
         * @param mesg_size          Size of the message data in bytes.
         */
        void WriteMessage(FIT_UINT8 local_mesg_number, const void* mesg_pointer, FIT_UINT16 mesg_size, SDK::Interface::IFile* fp);

        struct MsgField {
            uint8_t offset;
            uint8_t size;
        };

        bool                       mInited;
        const uint8_t              mMsgID;
        FIT_MESG_DEF*              mMsgDefOrigin;
        std::unique_ptr<uint8_t[]> mMsgDefBuffer;
        FIT_MESG_DEF*              mMsgDef;
        std::vector<MsgField>      mMsgFields;
        const bool                 mIsField;
        FIT_FIT_BASE_TYPE          mBaseType;
        const uint8_t              mDevelopItemsCount;
        const FIT_UINT8            mDevelopIndex;
		std::vector<FitHelper*>    mUserFields;
    };

}

#endif
