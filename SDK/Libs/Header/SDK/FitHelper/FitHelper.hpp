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

namespace SDK::Component {

    class FitHelper
    {
    public:
        FitHelper(const FIT_MESG_DEF& msgDef, uint16_t msgDefSize);

        bool init(std::initializer_list<FIT_EVENT_FIELD_NUM> fields);

        bool writeDef(SDK::Interface::IFile* fp);
        bool writeData(void* data, SDK::Interface::IFile* fp);

    private:
        typedef struct
        {
            FIT_UINT8  reserved_1;
            FIT_UINT8  arch;
            FIT_UINT16 global_mesg_num;
            FIT_UINT8  num_fields;
            FIT_UINT8* fields;
        } MESG_DEF;

        bool isUnique(std::initializer_list<FIT_EVENT_FIELD_NUM>& fields);

        const FIT_MESG_DEF&          mMsgDefOrigin;
        const uint16_t               mMsgDefOriginSize;
		MESG_DEF                     mMesgDef;
        std::unique_ptr<FIT_UINT8[]> mFields;
    };

}

#endif
