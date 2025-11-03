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
        FitHelper(uint8_t msgID, FIT_MESG_DEF* msgDef);

        bool init(std::initializer_list<FIT_UINT8> fields = {});

        bool writeDef(SDK::Interface::IFile* fp);
        
        bool writeMessage(void* data, SDK::Interface::IFile* fp);

        void printMsgDef(const FIT_MESG_DEF* msgDef);

		void addField(FitHelper* field);

		void writeFieldMessage(uint8_t idx, const void* data, SDK::Interface::IFile* fp);

        FIT_UINT8         getBaseTypeSize();
        FIT_FIT_BASE_TYPE getBaseType();

    private:
        FitHelper(const FitHelper&)            = delete;
        FitHelper& operator=(const FitHelper&) = delete;
        
        FitHelper(FitHelper&&)            = delete;
        FitHelper& operator=(FitHelper&&) = delete;

        FIT_UINT8 getBaseTypeSize(FIT_FIT_BASE_TYPE base_type);

        bool isUnique(std::initializer_list<FIT_EVENT_FIELD_NUM> fields);
        bool isValid(std::initializer_list<FIT_EVENT_FIELD_NUM> fields);
        void makeMsgDef(std::initializer_list<FIT_EVENT_FIELD_NUM> fields);
        void makeMsgFields(std::initializer_list<FIT_EVENT_FIELD_NUM> fields);

        uint16_t getFieldOffset(FIT_EVENT_FIELD_NUM field);
        uint16_t getFieldSize(FIT_EVENT_FIELD_NUM field);

        void WriteData(const void* data, FIT_UINT16 data_size, SDK::Interface::IFile* fp);
        void WriteMessageDefinition(FIT_UINT8              local_mesg_number,
                                    const void*            mesg_def_pointer,
                                    FIT_UINT16             mesg_def_size,
                                    SDK::Interface::IFile* fp);
        void WriteMessageDefinitionWithDevFields(FIT_UINT8              local_mesg_number,
                                                 const void*            mesg_def_pointer,
                                                 FIT_UINT16             mesg_def_size,
                                                 FIT_UINT8              number_dev_fields,
                                                 FIT_DEV_FIELD_DEF*     dev_field_definitions,
                                                 SDK::Interface::IFile* fp);
        void WriteMessage(FIT_UINT8 local_mesg_number, const void* mesg_pointer, FIT_UINT16 mesg_size, SDK::Interface::IFile* fp);
        void WriteDeveloperField(const void* data, FIT_UINT16 data_size, SDK::Interface::IFile* fp);

        struct MsgField {
            uint16_t offset;
            uint16_t size;
        };

        bool                       mInited;
        uint8_t                    mMsgID;
        FIT_MESG_DEF*              mMsgDefOrigin;
        std::unique_ptr<uint8_t[]> mMsgDefBuffer;
        FIT_MESG_DEF*              mMsgDef;
        std::vector<MsgField>      mMsgFields;
        bool                       mIsField;
        FIT_FIT_BASE_TYPE          mBaseType;
		std::vector<FitHelper*>    mFields;
    };

}

#endif
