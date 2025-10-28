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

FitHelper::FitHelper(const FIT_MESG_DEF& msgDef,
                     uint16_t            msgDefSize)
	: mMsgDefOrigin(msgDef)
	, mMsgDefOriginSize(msgDefSize)
    , mMesgDef()
    , mFields()
{
}

bool FitHelper::init(std::initializer_list<FIT_EVENT_FIELD_NUM> fields)
{
    if (!isUnique(fields)) {
         return false;
    }

	mFields = std::make_unique<FIT_UINT8[]>(fields.size() * FIT_FIELD_DEF_SIZE);
	mMesgDef.fields = mFields.get();

    mMesgDef.reserved_1      = mMsgDefOrigin.reserved_1;
	mMesgDef.arch            = mMsgDefOrigin.arch;
	mMesgDef.global_mesg_num = mMsgDefOrigin.global_mesg_num;
	mMesgDef.num_fields      = static_cast<FIT_UINT8>(fields.size());

    for (auto f : fields) {

    }

    return true;
}

bool FitHelper::writeDef(SDK::Interface::IFile* fp)
{
    return false;
}

bool FitHelper::writeData(void* data, SDK::Interface::IFile * fp)
{
    Fit_InitMesg((const FIT_MESG_DEF*)&mMesgDef, data);

	return false;
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

}